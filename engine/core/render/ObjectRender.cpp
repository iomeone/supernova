#include "ObjectRender.h"

#include "Engine.h"
#include "gles2/GLES2Object.h"

using namespace Supernova;


ObjectRender::ObjectRender(){
    minBufferSize = 0;
    primitiveType = 0;
    vertexBuffers.clear();
    vertexAttributes.clear();
    textures.clear();
    indexAttribute.data = NULL;
    properties.clear();
    programShader = -1;
    programDefs = 0;

    vertexSize = 0;
    numLights = 0;
    numShadows2D = 0;
    numShadowsCube = 0;
    
    sceneRender = NULL;
    lightRender = NULL;
    fogRender = NULL;

    program = NULL;
    parent = NULL;
}

ObjectRender* ObjectRender::newInstance(){
    if (Engine::getRenderAPI() == S_GLES2){
        return new GLES2Object();
    }
    
    return NULL;
}

ObjectRender::~ObjectRender(){
    program.reset();

    if (lightRender)
        delete lightRender;

    if (fogRender)
        delete fogRender;
}

void ObjectRender::setProgram(std::shared_ptr<ProgramRender> program){
    this->program = program;
}

void ObjectRender::setParent(ObjectRender* parent){
    this->parent = parent;
    this->program = parent->getProgram();
}

void ObjectRender::setSceneRender(SceneRender* sceneRender){
    this->sceneRender = sceneRender;
}

void ObjectRender::setLightRender(ObjectRender* lightRender){
    if (lightRender){
        if (!this->lightRender)
            this->lightRender = ObjectRender::newInstance();
        this->lightRender->properties = lightRender->properties;
    }
}

void ObjectRender::setFogRender(ObjectRender* fogRender) {
    if (fogRender){
        if (!this->fogRender)
            this->fogRender = ObjectRender::newInstance();
        this->fogRender->properties = fogRender->properties;
    }
}

void ObjectRender::setVertexSize(unsigned int vertexSize){
    this->vertexSize = vertexSize;
}

void ObjectRender::setMinBufferSize(unsigned int minBufferSize){
    this->minBufferSize = minBufferSize;
}

void ObjectRender::setPrimitiveType(int primitiveType){
    this->primitiveType = primitiveType;
}

void ObjectRender::setProgramShader(int programShader){
    this->programShader = programShader;
}

void ObjectRender::setProgramDefs(int programDefs){
    this->programDefs = programDefs;
}

void ObjectRender::addVertexBuffer(std::string name, unsigned int size, void* data, bool dynamic){
    if (data && (size > 0))
        vertexBuffers[name] = { size, data, dynamic };
}

void ObjectRender::addVertexAttribute(int type, std::string buffer, unsigned int elements, unsigned int stride, size_t offset){
    if ((!buffer.empty()) && (elements > 0))
        vertexAttributes[type] = { buffer, elements, stride, offset};
}

void ObjectRender::addIndex(unsigned int size, void* data, bool dynamic){
    if (data && (size > 0))
        indexAttribute = { size, data, dynamic };
}

void ObjectRender::addProperty(int type, int datatype, unsigned int size, void* data){
    if (data && (size > 0))
        properties[type] = { datatype, size, data };
}

void ObjectRender::addTexture(int type, Texture* texture){
    if (texture) {
        textures[type].clear();
        textures[type].push_back(texture);
    }
}

void ObjectRender::addTextureVector(int type, std::vector<Texture*> texturesVec){
    textures[type] = texturesVec;
}

void ObjectRender::updateVertexBuffer(std::string name, unsigned int size, void* data){
    if (vertexBuffers.count(name))
        addVertexBuffer(name, size, data);
}

void ObjectRender::updateIndex(unsigned int size, void* data){
    if (indexAttribute.data)
        addIndex(size, data);
}

void ObjectRender::setNumLights(int numLights){
    this->numLights = numLights;
}

void ObjectRender::setNumShadows2D(int numShadows2D){
    this->numShadows2D = numShadows2D;
}

void ObjectRender::setNumShadowsCube(int numShadowsCube){
    this->numShadowsCube = numShadowsCube;
}

std::shared_ptr<ProgramRender> ObjectRender::getProgram(){
    
    loadProgram();
    
    return program;
}

void ObjectRender::checkLighting(){
    if (lightRender == NULL || (programDefs & S_PROGRAM_IS_SKY)){
        numLights = 0;
    }
}

void ObjectRender::checkFog(){
    if (fogRender != NULL){
        programDefs |= S_PROGRAM_USE_FOG;
    }
}

void ObjectRender::checkTextureCoords(){
    if (textures.count(S_TEXTURESAMPLER_DIFFUSE))
        if (textures[S_TEXTURESAMPLER_DIFFUSE].size() > 0)
            programDefs |= S_PROGRAM_USE_TEXCOORD;
}

void ObjectRender::checkTextureRect(){
    if (vertexAttributes.count(S_VERTEXATTRIBUTE_TEXTURERECTS)){
        programDefs |= S_PROGRAM_USE_TEXRECT;
    }
    if (properties.count(S_PROPERTY_TEXTURERECT)){
        programDefs |= S_PROGRAM_USE_TEXRECT;
    }
}

void ObjectRender::checkTextureCube(){
    if (textures.count(S_TEXTURESAMPLER_DIFFUSE)) {
        for (size_t i = 0; i < textures[S_TEXTURESAMPLER_DIFFUSE].size(); i++) {
            if (textures[S_TEXTURESAMPLER_DIFFUSE][i]->getType() == S_TEXTURE_CUBE)
                programDefs |= S_PROGRAM_USE_TEXCUBE;
        }
    }
}

void ObjectRender::loadProgram(){
    checkLighting();
    checkFog();
    checkTextureCoords();
    checkTextureRect();
    checkTextureCube();

    std::string shaderStr = std::to_string(programShader);
    shaderStr += "|" + std::to_string(programDefs);
    shaderStr += "|" + std::to_string(numLights);
    shaderStr += "|" + std::to_string(numShadows2D);
    shaderStr += "|" + std::to_string(numShadowsCube);

    if (!program){
        program = ProgramRender::sharedInstance(shaderStr);

        if (!program.get()->isLoaded()){

            program.get()->createProgram(programShader, programDefs, numLights, numShadows2D, numShadowsCube);

        }
    }

}

bool ObjectRender::load(){
    
    loadProgram();
    for ( const auto &p : textures ) {
        for (size_t i = 0; i < p.second.size(); i++) {
            p.second[i]->load();
        }
    }

    if (numLights > 0){
        lightRender->setProgram(program);
        lightRender->load();
    }

    if (programDefs & S_PROGRAM_USE_FOG){
        fogRender->setProgram(program);
        fogRender->load();
    }
    
    return true;
}

bool ObjectRender::prepareDraw(){
    
    //lightRender and fogRender need to be called after main rander use program
    if (numLights > 0){
        lightRender->prepareDraw();
    }
    
    if (programDefs & S_PROGRAM_USE_FOG){
        fogRender->prepareDraw();
    }

    return true;
}

bool ObjectRender::draw(){

    return true;
}

bool ObjectRender::finishDraw(){
    return true;
}

void ObjectRender::destroy(){

    for (size_t i = 0; i < textures[S_TEXTURESAMPLER_DIFFUSE].size(); i++) {
        textures[S_TEXTURESAMPLER_DIFFUSE][i]->destroy();
    }

    program.reset();
    ProgramRender::deleteUnused();
}
