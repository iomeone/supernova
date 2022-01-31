#ifndef TEXTURE_H
#define TEXTURE_H

#include "render/TextureRender.h"
#include "render/FramebufferRender.h"
#include "texture/TextureData.h"
#include "pool/TexturePool.h"
#include <string>
#include <memory>

namespace Supernova{

    class Texture{
        private:
            std::shared_ptr<TexturePoolData> renderAndData = NULL;
            FramebufferRender* framebuffer;

            TextureType type;
            std::string id;

            std::string paths[6];
            TextureData data[6];

            bool loadFromPath;
            bool releaseDataAfterLoad;
            bool needLoad;

        public:
            Texture();
            Texture(std::string path);
            Texture(TextureData data, std::string id);

            void setPath(std::string path);
            void setData(TextureData data, std::string id);
            void setCubePath(size_t index, std::string path);
            void setCubePaths(std::string front, std::string back, std::string left, std::string right, std::string up, std::string down);
            void setFramebuffer(FramebufferRender* framebuffer);

            Texture(const Texture& rhs);
            Texture& operator=(const Texture& rhs);

            virtual ~Texture();

            bool load();
            void destroy();

            TextureRender* getRender();
            TextureData& getData(size_t index = 0);

            void setReleaseDataAfterLoad(bool releaseDataAfterLoad);

            bool empty();
            bool hasTextureFrame();
    };
}

#endif //TEXTURE_H