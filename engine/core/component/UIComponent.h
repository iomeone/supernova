//
// (c) 2022 Eduardo Doria.
//

#ifndef UI_COMPONENT_H
#define UI_COMPONENT_H

#include "buffer/Buffer.h"
#include "texture/Texture.h"
#include "render/ObjectRender.h"
#include "buffer/InterleavedBuffer.h"
#include "buffer/IndexBuffer.h"

namespace Supernova{

    struct UIComponent{
        // Render part
        bool loaded = false;

        InterleavedBuffer buffer;
        IndexBuffer indices;

        unsigned int minBufferCount = 0;
        unsigned int minIndicesCount = 0;

        ObjectRender render;
        std::shared_ptr<ShaderRender> shader;
        std::string shaderProperties;
        int slotVSParams = -1;
        int slotFSParams = -1;

        PrimitiveType primitiveType = PrimitiveType::TRIANGLES;
        unsigned int vertexCount = 0;

        Texture texture;
        Vector4 color = Vector4(1.0, 1.0, 1.0, 1.0); //linear color

        bool needReload = false;

        bool needUpdateBuffer = false;
        bool needUpdateTexture = false;
    };

}

#endif //UI_COMPONENT_H