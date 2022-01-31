//
// (c) 2021 Eduardo Doria.
//

#include "SBSReader.h"

#include "io/File.h"
#include "Log.h"

#define makefourcc(_a, _b, _c, _d) (((uint32_t)(_a) | ((uint32_t)(_b) << 8) | ((uint32_t)(_c) << 16) | ((uint32_t)(_d) << 24)))

#define SBS_NAME_SIZE 32

//----------Begin SBS format---------------
#pragma pack(push, 1)

#define SBS_CHUNK               makefourcc('S', 'B', 'S', ' ')
#define SBS_CHUNK_STAG          makefourcc('S', 'T', 'A', 'G')
#define SBS_CHUNK_CODE          makefourcc('C', 'O', 'D', 'E')
#define SBS_CHUNK_DATA          makefourcc('D', 'A', 'T', 'A')
#define SBS_CHUNK_REFL          makefourcc('R', 'E', 'F', 'L')

#define SBS_STAGE_VERTEX        makefourcc('V', 'E', 'R', 'T')
#define SBS_STAGE_FRAGMENT      makefourcc('F', 'R', 'A', 'G')

#define SBS_LANG_HLSL           makefourcc('H', 'L', 'S', 'L')
#define SBS_LANG_GLSL           makefourcc('G', 'L', 'S', 'L')
#define SBS_LANG_MSL            makefourcc('M', 'S', 'L', ' ')

#define SBS_VERTEXTYPE_FLOAT    makefourcc('F', 'L', 'T', '1')
#define SBS_VERTEXTYPE_FLOAT2   makefourcc('F', 'L', 'T', '2')
#define SBS_VERTEXTYPE_FLOAT3   makefourcc('F', 'L', 'T', '3')
#define SBS_VERTEXTYPE_FLOAT4   makefourcc('F', 'L', 'T', '4')
#define SBS_VERTEXTYPE_INT      makefourcc('I', 'N', 'T', '1')
#define SBS_VERTEXTYPE_INT2     makefourcc('I', 'N', 'T', '2')
#define SBS_VERTEXTYPE_INT3     makefourcc('I', 'N', 'T', '3')
#define SBS_VERTEXTYPE_INT4     makefourcc('I', 'N', 'T', '4')

#define SBS_UNIFORMTYPE_FLOAT    makefourcc('F', 'L', 'T', '1')
#define SBS_UNIFORMTYPE_FLOAT2   makefourcc('F', 'L', 'T', '2')
#define SBS_UNIFORMTYPE_FLOAT3   makefourcc('F', 'L', 'T', '3')
#define SBS_UNIFORMTYPE_FLOAT4   makefourcc('F', 'L', 'T', '4')
#define SBS_UNIFORMTYPE_INT      makefourcc('I', 'N', 'T', '1')
#define SBS_UNIFORMTYPE_INT2     makefourcc('I', 'N', 'T', '2')
#define SBS_UNIFORMTYPE_INT3     makefourcc('I', 'N', 'T', '3')
#define SBS_UNIFORMTYPE_INT4     makefourcc('I', 'N', 'T', '4')
#define SBS_UNIFORMTYPE_MAT3     makefourcc('M', 'A', 'T', '3')
#define SBS_UNIFORMTYPE_MAT4     makefourcc('M', 'A', 'T', '4')

#define SBS_TEXTURE_2D          makefourcc('2', 'D', ' ', ' ')
#define SBS_TEXTURE_3D          makefourcc('3', 'D', ' ', ' ')
#define SBS_TEXTURE_CUBE        makefourcc('C', 'U', 'B', 'E')
#define SBS_TEXTURE_ARRAY       makefourcc('A', 'R', 'R', 'A')

#define SBS_SAMPLERTYPE_FLOAT   makefourcc('T', 'F', 'L', 'T')
#define SBS_SAMPLERTYPE_SINT    makefourcc('T', 'I', 'N', 'T')
#define SBS_SAMPLERTYPE_UINT    makefourcc('T', 'U', 'I', 'T')

struct sbs_chunk {
    uint32_t sbs_version;
    uint32_t lang;
    uint32_t version;
    uint16_t es;
};

// STAG
struct sbs_stage {
    uint32_t type;
};

// REFL
struct sbs_chunk_refl {
    char     name[SBS_NAME_SIZE];
    uint32_t num_inputs;
    uint32_t num_textures;
    uint32_t num_uniform_blocks;
    uint32_t num_uniforms;
};

struct sbs_refl_input {
    char     name[SBS_NAME_SIZE];
    int32_t  location;
    char     semantic_name[SBS_NAME_SIZE];
    uint32_t semantic_index;
    uint32_t type;
};

struct sbs_refl_texture {
    char     name[SBS_NAME_SIZE];
    uint32_t set;
    int32_t  binding;
    uint32_t type;
    uint32_t sampler_type;
}; 

struct sbs_refl_uniformblock {
    uint32_t num_uniforms;
    char     name[SBS_NAME_SIZE];
    char     inst_name[SBS_NAME_SIZE];
    uint32_t set;
    int32_t  binding;
    uint32_t size_bytes;
    bool     flattened;
};

struct sbs_refl_uniform {
    char     name[SBS_NAME_SIZE];
    uint32_t type;
    uint32_t array_count;
    uint32_t offset;
};

#pragma pack(pop)
//----------End SBS format---------------


using namespace Supernova;


SBSReader::SBSReader(){

}

SBSReader::~SBSReader(){
    for (int i = 0; i < shaderData.stages.size(); i++){
        if (shaderData.stages[i].bytecode.data)
            delete shaderData.stages[i].bytecode.data;
    }
}

bool SBSReader::read(std::string filepath){
    File file;

    if (file.open(filepath.c_str()) != FileErrors::NO_ERROR){
        Log::Error("Cannot open sbs file: %s", filepath.c_str());
        return false;
    }

    uint32_t sbs = file.read32();
    if (sbs != SBS_CHUNK) {
        Log::Error("Invalid sbs file format: %s", filepath.c_str());
        return false;
    }

    file.seek(file.pos()+sizeof(uint32_t));

    sbs_chunk sinfo;
    file.read((unsigned char*)&sinfo, sizeof(sinfo));

    if (sinfo.sbs_version != 100){
        Log::Error("Invalid sbs file version: %s", filepath.c_str());
        return false;
    }

    shaderData.version = sinfo.version;
    shaderData.es = sinfo.es;

    if (sinfo.lang == SBS_LANG_GLSL){
        shaderData.lang = ShaderLang::GLSL;
    }else if (sinfo.lang == SBS_LANG_HLSL){
        shaderData.lang = ShaderLang::HLSL;
    }else if (sinfo.lang == SBS_LANG_MSL){
        shaderData.lang = ShaderLang::MSL;
    }

    uint32_t stag = file.read32();
    uint32_t stagSize = file.read32();

    ShaderStage* shaderStage;

    while (stag == SBS_CHUNK_STAG){
        uint32_t stagPos = file.pos();

        sbs_stage sbsstage;
        file.read((unsigned char*)&sbsstage, sizeof(sbs_stage));

        shaderData.stages.push_back({});
        shaderStage = &shaderData.stages.back();
        if (sbsstage.type == SBS_STAGE_VERTEX) {
            shaderStage->type = ShaderStageType::VERTEX;
        } else if (sbsstage.type == SBS_STAGE_FRAGMENT) {
            shaderStage->type = ShaderStageType::FRAGMENT;
        } else {
            Log::Error("Stage not implemented: %s", filepath.c_str());
            return false;
        }

        uint32_t code = file.read32();
        uint32_t codeSize = file.read32();

        if (code == SBS_CHUNK_CODE){
            shaderStage->source = file.readString(codeSize);
        }else if (code == SBS_CHUNK_DATA){
            shaderStage->bytecode.data = new unsigned char[codeSize];
            shaderStage->bytecode.size = codeSize;
            file.read((unsigned char*)&shaderStage->bytecode.data, codeSize);
        }

        uint32_t refl = file.read32();
        uint32_t reflSize = file.read32();

        if (refl == SBS_CHUNK_REFL){
            sbs_chunk_refl refl_chunk;
            file.read((unsigned char*)&refl_chunk, sizeof(refl_chunk));

            shaderStage->name = std::string(refl_chunk.name);

            for (uint32_t i = 0; i < refl_chunk.num_inputs; i++) {
                sbs_refl_input in;
                file.read((unsigned char*)&in, sizeof(in));

                ShaderAttr attr;
                attr.location = in.location;
                attr.name = std::string(in.name);
                attr.semanticName = std::string(in.semantic_name);
                attr.semanticIndex = in.semantic_index;
                if (in.type == SBS_VERTEXTYPE_FLOAT){
                    attr.type = ShaderVertexType::FLOAT;
                }else if (in.type == SBS_VERTEXTYPE_FLOAT2){
                    attr.type = ShaderVertexType::FLOAT2;
                }else if (in.type == SBS_VERTEXTYPE_FLOAT3){
                    attr.type = ShaderVertexType::FLOAT3;
                }else if (in.type == SBS_VERTEXTYPE_FLOAT4){
                    attr.type = ShaderVertexType::FLOAT4;
                }else if (in.type == SBS_VERTEXTYPE_INT){
                    attr.type = ShaderVertexType::INT;
                }else if (in.type == SBS_VERTEXTYPE_INT2){
                    attr.type = ShaderVertexType::INT2;
                }else if (in.type == SBS_VERTEXTYPE_INT3){
                    attr.type = ShaderVertexType::INT3;
                }else if (in.type == SBS_VERTEXTYPE_INT4){
                    attr.type = ShaderVertexType::INT4;
                }

                shaderStage->attributes.push_back(attr);
            }

            for (uint32_t i = 0; i < refl_chunk.num_textures; i++) {
                sbs_refl_texture t;
                file.read((unsigned char*)&t, sizeof(t));

                ShaderTexture texture;
                texture.name = std::string(t.name);
                texture.set = t.set;
                texture.binding = t.binding;

                if (t.type == SBS_TEXTURE_2D){
                    texture.type = TextureType::TEXTURE_2D;
                }else if (t.type == SBS_TEXTURE_3D){
                    texture.type = TextureType::TEXTURE_3D;
                }else if (t.type == SBS_TEXTURE_CUBE){
                    texture.type = TextureType::TEXTURE_CUBE;
                }else if (t.type == SBS_TEXTURE_ARRAY){
                    texture.type = TextureType::TEXTURE_ARRAY;
                }else{
                    texture.type = TextureType::TEXTURE_2D;
                }

                if (t.sampler_type == SBS_SAMPLERTYPE_FLOAT){
                    texture.samplerType = TextureSamplerType::FLOAT;
                }else if (t.sampler_type == SBS_SAMPLERTYPE_UINT){
                    texture.samplerType = TextureSamplerType::UINT;
                }else if (t.sampler_type == SBS_SAMPLERTYPE_SINT){
                    texture.samplerType = TextureSamplerType::SINT;
                }else{
                    texture.samplerType = TextureSamplerType::FLOAT;
                }

                shaderStage->textures.push_back(texture);
            }

            for (uint32_t i = 0; i < refl_chunk.num_uniform_blocks; i++) {
                sbs_refl_uniformblock ub;
                file.read((unsigned char*)&ub, sizeof(ub));

                ShaderUniformBlock uniformblock;
                uniformblock.name = std::string(ub.name);
                uniformblock.instName = std::string(ub.inst_name);
                uniformblock.set = ub.set;
                uniformblock.binding = ub.binding;
                uniformblock.sizeBytes = ub.size_bytes;
                uniformblock.flattened = ub.flattened;

                for (uint32_t j = 0; j < ub.num_uniforms; j++) {
                    sbs_refl_uniform u;
                    file.read((unsigned char*)&u, sizeof(u));

                    ShaderUniform uniform;
                    uniform.name = std::string(ub.inst_name) + "." + std::string(u.name);
                    uniform.arrayCount = u.array_count;
                    uniform.offset = u.offset;
                    if (u.type == SBS_UNIFORMTYPE_FLOAT){
                        uniform.type = ShaderUniformType::FLOAT;
                    }else if (u.type == SBS_UNIFORMTYPE_FLOAT2){
                        uniform.type = ShaderUniformType::FLOAT2;
                    }else if (u.type == SBS_UNIFORMTYPE_FLOAT3){
                        uniform.type = ShaderUniformType::FLOAT3;
                    }else if (u.type == SBS_UNIFORMTYPE_FLOAT4){
                        uniform.type = ShaderUniformType::FLOAT4;
                    }else if (u.type == SBS_UNIFORMTYPE_INT){
                        uniform.type = ShaderUniformType::INT;
                    }else if (u.type == SBS_UNIFORMTYPE_INT2){
                        uniform.type = ShaderUniformType::INT2;
                    }else if (u.type == SBS_UNIFORMTYPE_INT3){
                        uniform.type = ShaderUniformType::INT3;
                    }else if (u.type == SBS_UNIFORMTYPE_INT4){
                        uniform.type = ShaderUniformType::INT4;
                    }else if (u.type == SBS_UNIFORMTYPE_MAT3){
                        uniform.type = ShaderUniformType::MAT3;
                    }else if (u.type == SBS_UNIFORMTYPE_MAT4){
                        uniform.type = ShaderUniformType::MAT4;
                    }

                    uniformblock.uniforms.push_back(uniform);
                }

                shaderStage->uniformblocks.push_back(uniformblock);
            }
        }

        //Next STAG
        file.seek(stagPos + stagSize);
        stag = file.read32();
        stagSize = file.read32();
    }

    return true;
}

ShaderData& SBSReader::getShaderData(){
    return shaderData;
}