//
// (c) 2018 Eduardo Doria.
//

#ifndef ATTRIBUTEBUFFER_H
#define ATTRIBUTEBUFFER_H

#include <string>
#include <vector>
#include <map>

#include "render/ProgramRender.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"

namespace Supernova {

    struct AttributeData{
        int count;
        int offset;
        int elements;
    };

    class AttributeBuffer {

    private:
        std::string name;
        std::vector<float> buffer;
        unsigned int itemSize;
        unsigned int count;
        std::map<int, AttributeData> attributes;

    public:
        AttributeBuffer();
        virtual ~AttributeBuffer();

        void addAttribute(int attribute, int elements);
        AttributeData* getAttribute(int attribute);

        void clearAll();
        void clearBuffer();


        void addValue(int attribute, float value);
        void addValue(int attribute, Vector2 vector);
        void addValue(int attribute, Vector3 vector);
        void addValue(int attribute, Vector4 vector);

        void addValue(AttributeData* attribute, float value);
        void addValue(AttributeData* attribute, Vector2 vector);
        void addValue(AttributeData* attribute, Vector3 vector);
        void addValue(AttributeData* attribute, Vector4 vector);

        void setValue(unsigned int index, AttributeData* attribute, float value);
        void setValue(unsigned int index, AttributeData* attribute, Vector2 vector);
        void setValue(unsigned int index, AttributeData* attribute, Vector3 vector);
        void setValue(unsigned int index, AttributeData* attribute, Vector4 vector);
        void setValue(unsigned int index, AttributeData* attribute, unsigned int numValues, float* vector);

        Vector2 getValueVector2(int attribute, unsigned int index);
        Vector3 getValueVector3(int attribute, unsigned int index);
        Vector4 getValueVector4(int attribute, unsigned int index);

        Vector2 getValueVector2(AttributeData* attribute, unsigned int index);
        Vector3 getValueVector3(AttributeData* attribute, unsigned int index);
        Vector4 getValueVector4(AttributeData* attribute, unsigned int index);

        float getValue(int attribute, unsigned int index);
        float getValue(AttributeData* attribute, unsigned int index, int elementIndex = 0);

        std::map<int, AttributeData> getAttributes();
        float* getBuffer();
        unsigned int getSize();

        unsigned int getItemSize();
        unsigned int getCount();

        const std::string &getName() const;
        void setName(const std::string &name);

    };

}


#endif //ATTRIBUTEBUFFER_H
