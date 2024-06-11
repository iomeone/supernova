//
// (c) 2024 Eduardo Doria.
//

#include "UISystem.h"

#include <locale>
#include <codecvt>
#include <algorithm>
#include "Scene.h"
#include "Input.h"
#include "Engine.h"
#include "System.h"
#include "util/STBText.h"
#include "util/StringUtils.h"
#include "pool/FontPool.h"

using namespace Supernova;


UISystem::UISystem(Scene* scene): SubSystem(scene){
    signature.set(scene->getComponentType<UILayoutComponent>());

    eventId.clear();
    lastUIFromPointer = NULL_ENTITY;
    lastPanelFromPointer = NULL_ENTITY;
    lastPointerPos = Vector2(-1, -1);
}

UISystem::~UISystem(){
}

bool UISystem::createImagePatches(ImageComponent& img, UIComponent& ui, UILayoutComponent& layout){

    ui.texture.load();
    unsigned int texWidth = ui.texture.getWidth();
    unsigned int texHeight = ui.texture.getHeight();

    if (texWidth == 0 || texHeight == 0){
        texWidth = layout.width;
        texHeight = layout.height;
    }

    if (layout.width == 0 && layout.height == 0){
        layout.width = texWidth;
        layout.height = texHeight;
    }

    if ((layout.width == 0 || layout.height == 0) && layout.anchorPreset == AnchorPreset::NONE){
        Log::warn("Cannot create UI image without size");
        return false;
    }

    ui.primitiveType = PrimitiveType::TRIANGLES;

	ui.buffer.clear();
	ui.buffer.addAttribute(AttributeType::POSITION, 3);
	ui.buffer.addAttribute(AttributeType::TEXCOORD1, 2);
    ui.buffer.addAttribute(AttributeType::COLOR, 4);
    ui.buffer.setUsage(BufferUsage::DYNAMIC);

    ui.indices.clear();
    ui.indices.setUsage(BufferUsage::DYNAMIC);

    Attribute* atrVertex = ui.buffer.getAttribute(AttributeType::POSITION);

    ui.buffer.addVector3(atrVertex, Vector3(0, 0, 0)); //0
    ui.buffer.addVector3(atrVertex, Vector3(layout.width, 0, 0)); //1
    ui.buffer.addVector3(atrVertex, Vector3(layout.width,  layout.height, 0)); //2
    ui.buffer.addVector3(atrVertex, Vector3(0,  layout.height, 0)); //3

    ui.buffer.addVector3(atrVertex, Vector3(img.patchMarginLeft, img.patchMarginTop, 0)); //4
    ui.buffer.addVector3(atrVertex, Vector3(layout.width-img.patchMarginRight, img.patchMarginTop, 0)); //5
    ui.buffer.addVector3(atrVertex, Vector3(layout.width-img.patchMarginRight,  layout.height-img.patchMarginBottom, 0)); //6
    ui.buffer.addVector3(atrVertex, Vector3(img.patchMarginLeft,  layout.height-img.patchMarginBottom, 0)); //7

    ui.buffer.addVector3(atrVertex, Vector3(img.patchMarginLeft, 0, 0)); //8
    ui.buffer.addVector3(atrVertex, Vector3(0, img.patchMarginTop, 0)); //9

    ui.buffer.addVector3(atrVertex, Vector3(layout.width-img.patchMarginRight, 0, 0)); //10
    ui.buffer.addVector3(atrVertex, Vector3(layout.width, img.patchMarginTop, 0)); //11

    ui.buffer.addVector3(atrVertex, Vector3(layout.width-img.patchMarginRight, layout.height, 0)); //12
    ui.buffer.addVector3(atrVertex, Vector3(layout.width, layout.height-img.patchMarginBottom, 0)); //13

    ui.buffer.addVector3(atrVertex, Vector3(img.patchMarginLeft, layout.height, 0)); //14
    ui.buffer.addVector3(atrVertex, Vector3(0, layout.height-img.patchMarginBottom, 0)); //15

    Attribute* atrTexcoord = ui.buffer.getAttribute(AttributeType::TEXCOORD1);

    float texCutRatioW = 0;
    float texCutRatioH = 0;
    if (texWidth != 0 && texHeight != 0){
        texCutRatioW = 1.0 / texWidth * img.textureCutFactor;
        texCutRatioH = 1.0 / texHeight * img.textureCutFactor;
    }

    float x0 = texCutRatioW;
    float x1 = 1.0-texCutRatioW;
    float y0 = texCutRatioH;
    float y1 = 1.0-texCutRatioH;

    ui.buffer.addVector2(atrTexcoord, Vector2(x0, y0));
    ui.buffer.addVector2(atrTexcoord, Vector2(x1, y0));
    ui.buffer.addVector2(atrTexcoord, Vector2(x1, y1));
    ui.buffer.addVector2(atrTexcoord, Vector2(x0, y1));

    ui.buffer.addVector2(atrTexcoord, Vector2(img.patchMarginLeft/(float)texWidth, img.patchMarginTop/(float)texHeight));
    ui.buffer.addVector2(atrTexcoord, Vector2(x1-(img.patchMarginRight/(float)texWidth), img.patchMarginTop/(float)texHeight));
    ui.buffer.addVector2(atrTexcoord, Vector2(x1-(img.patchMarginRight/(float)texWidth), y1-(img.patchMarginBottom/(float)texHeight)));
    ui.buffer.addVector2(atrTexcoord, Vector2(img.patchMarginLeft/(float)texWidth, y1-(img.patchMarginBottom/(float)texHeight)));

    ui.buffer.addVector2(atrTexcoord, Vector2(img.patchMarginLeft/(float)texWidth, y0));
    ui.buffer.addVector2(atrTexcoord, Vector2(x0, img.patchMarginTop/(float)texHeight));

    ui.buffer.addVector2(atrTexcoord, Vector2(x1-(img.patchMarginRight/(float)texWidth), y0));
    ui.buffer.addVector2(atrTexcoord, Vector2(x1, img.patchMarginTop/(float)texHeight));

    ui.buffer.addVector2(atrTexcoord, Vector2(x1-(img.patchMarginRight/(float)texWidth), y1));
    ui.buffer.addVector2(atrTexcoord, Vector2(x1, y1-(img.patchMarginBottom/(float)texHeight)));

    ui.buffer.addVector2(atrTexcoord, Vector2((img.patchMarginLeft/(float)texWidth), y1));
    ui.buffer.addVector2(atrTexcoord, Vector2(x0, y1-(img.patchMarginBottom/(float)texHeight)));

    if (ui.flipY){
        for (int i = 0; i < ui.buffer.getCount(); i++){
            Vector2 uv = ui.buffer.getVector2(atrTexcoord, i);
            uv.y = 1.0 - uv.y;
            ui.buffer.setVector2(i, atrTexcoord, uv);
        }
    }

    Attribute* atrColor = ui.buffer.getAttribute(AttributeType::COLOR);

    for (int i = 0; i < ui.buffer.getCount(); i++){
        ui.buffer.addVector4(atrColor, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    static const uint16_t indices_array[] = {
        4,  5,  6, // Center
        4,  6,  7,

        0,  8,  4, // Top-left
        0,  4,  9,

        8, 10,  5, // Top
        8,  5,  4,

        10, 1, 11, // Top-right
        10, 11, 5,

        5, 11, 13, // Right
        5, 13,  6,

        6, 13,  2, // Bottom-right
        6,  2, 12,

        7,  6, 12, // Bottom
        7, 12, 14,

        15, 7, 14, // Bottom-left
        15, 14, 3,

        9,  4,  7, // Left
        9,  7, 15
    };

    ui.indices.setValues(
        0, ui.indices.getAttribute(AttributeType::INDEX),
        54, (char*)&indices_array[0], sizeof(uint16_t));

    if (ui.loaded)
        ui.needUpdateBuffer = true;

    return true;
}

bool UISystem::loadFontAtlas(TextComponent& text, UIComponent& ui, UILayoutComponent& layout){
    std::string fontId = text.font;
    if (text.font.empty())
        fontId = "font";
    fontId = fontId + std::string("|") + std::to_string(text.fontSize);

    text.stbtext = FontPool::get(fontId);
    if (!text.stbtext){
        text.stbtext = FontPool::get(fontId, text.font, text.fontSize);
        if (!text.stbtext) {
            Log::error("Cannot load font atlas from: %s", text.font.c_str());
            return false;
        }
    }

    ui.texture.setData(*text.stbtext->getTextureData(), fontId);
    ui.texture.setReleaseDataAfterLoad(true);

    ui.needUpdateTexture = true;

    text.needReload = false;
    text.loaded = true;

    return true;
}

void UISystem::createText(TextComponent& text, UIComponent& ui, UILayoutComponent& layout){

    ui.primitiveType = PrimitiveType::TRIANGLES;

    ui.buffer.clear();
	ui.buffer.addAttribute(AttributeType::POSITION, 3);
	ui.buffer.addAttribute(AttributeType::TEXCOORD1, 2);
    ui.buffer.setUsage(BufferUsage::DYNAMIC);

    ui.indices.clear();
    ui.indices.setUsage(BufferUsage::DYNAMIC);

    std::vector<uint16_t> indices_array;

    if (text.text.length() > text.maxTextSize){
        Log::warn("Text is bigger than maxTextSize: %i. Increasing it to: %i", text.maxTextSize, text.text.length());
        text.maxTextSize = text.text.length();
        if (ui.loaded){
            ui.needReload = true;
        }
    }

    ui.minBufferCount = text.maxTextSize * 4;
    ui.minIndicesCount = text.maxTextSize * 6;

    text.stbtext->createText(text.text, &ui.buffer, indices_array, text.charPositions, layout.width, layout.height, text.fixedWidth, text.fixedHeight, text.multiline, ui.flipY);

    if (text.pivotCentered || !text.pivotBaseline){
        Attribute* atrVertice = ui.buffer.getAttribute(AttributeType::POSITION);
        for (int i = 0; i < ui.buffer.getCount(); i++){
            Vector3 vertice = ui.buffer.getVector3(atrVertice, i);

            if (text.pivotCentered){
                vertice.x = vertice.x - (layout.width / 2.0);
            }

            if (!text.pivotBaseline){
                if (!ui.flipY){
                    vertice.y = vertice.y + text.stbtext->getAscent();
                }else{
                    vertice.y = vertice.y + layout.height - text.stbtext->getAscent();
                }
            }

            ui.buffer.setVector3(i, atrVertice, vertice);
        }
    }

    ui.indices.setValues(
            0, ui.indices.getAttribute(AttributeType::INDEX),
            indices_array.size(), (char*)&indices_array[0], sizeof(uint16_t));

    if (ui.loaded)
        ui.needUpdateBuffer = true;
}

void UISystem::createButtonObjects(Entity entity, ButtonComponent& button){
    if (button.label == NULL_ENTITY){
        button.label = scene->createEntity();

        scene->addComponent<Transform>(button.label, {});
        scene->addComponent<UILayoutComponent>(button.label, {});
        scene->addComponent<UIComponent>(button.label, {});
        scene->addComponent<TextComponent>(button.label, {});

        scene->addEntityChild(entity, button.label);

        UIComponent& labelui = scene->getComponent<UIComponent>(button.label);
        UILayoutComponent& labellayout = scene->getComponent<UILayoutComponent>(button.label);
        labelui.color = Vector4(0.0, 0.0, 0.0, 1.0);
        labellayout.ignoreEvents = true;
    }
}

void UISystem::createPanelObjects(Entity entity, PanelComponent& panel){
    if (panel.headerimage == NULL_ENTITY){
        panel.headerimage = scene->createEntity();

        scene->addComponent<Transform>(panel.headerimage, {});
        scene->addComponent<UILayoutComponent>(panel.headerimage, {});
        scene->addComponent<UIComponent>(panel.headerimage, {});
        scene->addComponent<ImageComponent>(panel.headerimage, {});

        scene->addEntityChild(entity, panel.headerimage);

        UILayoutComponent& headerimagelayout = scene->getComponent<UILayoutComponent>(panel.headerimage);
        headerimagelayout.height = 1;
        headerimagelayout.width = 1;
        headerimagelayout.ignoreEvents = true;
    }
    if (panel.headercontainer == NULL_ENTITY){
        panel.headercontainer = scene->createEntity();

        scene->addComponent<Transform>(panel.headercontainer, {});
        scene->addComponent<UILayoutComponent>(panel.headercontainer, {});
        scene->addComponent<UIContainerComponent>(panel.headercontainer, {});

        scene->addEntityChild(panel.headerimage, panel.headercontainer);

        UILayoutComponent& containerlayout = scene->getComponent<UILayoutComponent>(panel.headercontainer);
        containerlayout.ignoreEvents = true;
    }
    if (panel.headertext == NULL_ENTITY){
        panel.headertext = scene->createEntity();

        scene->addComponent<Transform>(panel.headertext, {});
        scene->addComponent<UILayoutComponent>(panel.headertext, {});
        scene->addComponent<UIComponent>(panel.headertext, {});
        scene->addComponent<TextComponent>(panel.headertext, {});

        scene->addEntityChild(panel.headercontainer, panel.headertext);

        UILayoutComponent& titlelayout = scene->getComponent<UILayoutComponent>(panel.headertext);
        titlelayout.ignoreEvents = true;
    }
}

void UISystem::createScrollbarObjects(Entity entity, ScrollbarComponent& scrollbar){
    if (scrollbar.bar == NULL_ENTITY){
        scrollbar.bar = scene->createEntity();

        scene->addComponent<Transform>(scrollbar.bar, {});
        scene->addComponent<UILayoutComponent>(scrollbar.bar, {});
        scene->addComponent<UIComponent>(scrollbar.bar, {});
        scene->addComponent<ImageComponent>(scrollbar.bar, {});

        scene->addEntityChild(entity, scrollbar.bar);

        UILayoutComponent& barlayout = scene->getComponent<UILayoutComponent>(scrollbar.bar);
        barlayout.height = 1;
        barlayout.width = 1;
        barlayout.ignoreEvents = true;
    }
}

void UISystem::createTextEditObjects(Entity entity, TextEditComponent& textedit){
    if (textedit.text == NULL_ENTITY){
        textedit.text = scene->createEntity();

        scene->addComponent<Transform>(textedit.text, {});
        scene->addComponent<UILayoutComponent>(textedit.text, {});
        scene->addComponent<UIComponent>(textedit.text, {});
        scene->addComponent<TextComponent>(textedit.text, {});

        scene->addEntityChild(entity, textedit.text);

        UIComponent& textui = scene->getComponent<UIComponent>(textedit.text);
        UILayoutComponent& textlayout = scene->getComponent<UILayoutComponent>(textedit.text);
        textui.color = Vector4(0.0, 0.0, 0.0, 1.0);
        textlayout.ignoreEvents = true;
    }

    if (textedit.cursor == NULL_ENTITY){
        textedit.cursor = scene->createEntity();

        scene->addComponent<Transform>(textedit.cursor, {});
        scene->addComponent<UILayoutComponent>(textedit.cursor, {});
        scene->addComponent<UIComponent>(textedit.cursor, {});
        scene->addComponent<PolygonComponent>(textedit.cursor, {});

        scene->addEntityChild(entity, textedit.cursor);

        UILayoutComponent& cursorlayout = scene->getComponent<UILayoutComponent>(textedit.cursor);
        cursorlayout.ignoreEvents = true;
    }
}

void UISystem::updateButton(Entity entity, ButtonComponent& button, ImageComponent& img, UIComponent& ui, UILayoutComponent& layout){
    createButtonObjects(entity, button);

    if (!ui.loaded){
        if (!button.textureNormal.load()){
            button.textureNormal = ui.texture;
        }
        button.textureNormal.load();
        button.texturePressed.load();
        button.textureDisabled.load();
    }

    Transform& labeltransform = scene->getComponent<Transform>(button.label);
    TextComponent& labeltext = scene->getComponent<TextComponent>(button.label);
    UIComponent& labelui = scene->getComponent<UIComponent>(button.label);
    UILayoutComponent& labellayout = scene->getComponent<UILayoutComponent>(button.label);

    labellayout.width = 0;
    labellayout.height = 0;
    labellayout.anchorPreset = AnchorPreset::CENTER;
    labellayout.usingAnchors = true;

    labeltext.needUpdateText = true;
    createOrUpdateText(labeltext, labelui, labellayout);

    if (button.disabled){
        if (ui.texture != button.textureDisabled){
            ui.texture = button.textureDisabled;
            ui.needUpdateTexture = true;
        }
    }else{
        if (!button.pressed){
            if (ui.texture != button.textureNormal){
                ui.texture = button.textureNormal;
                ui.needUpdateTexture = true;
            }
        }else{
            if (ui.texture != button.texturePressed){
                ui.texture = button.texturePressed;
                ui.needUpdateTexture = true;
            }
        }
    }
}

void UISystem::updatePanel(Entity entity, PanelComponent& panel, ImageComponent& img, UIComponent& ui, UILayoutComponent& layout){
    createPanelObjects(entity, panel);

    UILayoutComponent& headerimagelayout = scene->getComponent<UILayoutComponent>(panel.headerimage);

    headerimagelayout.anchorPreset = AnchorPreset::TOP_WIDE;
    headerimagelayout.ignoreScissor = true;
    headerimagelayout.usingAnchors = true;
    headerimagelayout.height = img.patchMarginTop - img.patchMarginBottom;

    UIContainerComponent& containerui = scene->getComponent<UIContainerComponent>(panel.headercontainer);
    UILayoutComponent& containerlayout = scene->getComponent<UILayoutComponent>(panel.headercontainer);

    containerlayout.anchorPreset = AnchorPreset::FULL_LAYOUT;
    containerlayout.ignoreScissor = true;
    containerlayout.usingAnchors = true;
    containerui.type = ContainerType::HORIZONTAL;

    Transform& titletransform = scene->getComponent<Transform>(panel.headertext);
    TextComponent& headertext = scene->getComponent<TextComponent>(panel.headertext);
    UIComponent& titleui = scene->getComponent<UIComponent>(panel.headertext);
    UILayoutComponent& titlelayout = scene->getComponent<UILayoutComponent>(panel.headertext);

    titleui.color = Vector4(0.0, 0.0, 0.0, 1.0);
    titlelayout.width = 0;
    titlelayout.height = 0;
    titlelayout.anchorPreset = panel.titleAnchorPreset;
    titlelayout.ignoreScissor = true;
    titlelayout.usingAnchors = true;

    headertext.needUpdateText = true;
    createOrUpdateText(headertext, titleui, titlelayout);
}

void UISystem::updateScrollbar(Entity entity, ScrollbarComponent& scrollbar, ImageComponent& img, UIComponent& ui, UILayoutComponent& layout){
    createScrollbarObjects(entity, scrollbar);

    Transform& bartransform = scene->getComponent<Transform>(scrollbar.bar);
    ImageComponent& barimage = scene->getComponent<ImageComponent>(scrollbar.bar);
    UIComponent& barui = scene->getComponent<UIComponent>(scrollbar.bar);
    UILayoutComponent& barlayout = scene->getComponent<UILayoutComponent>(scrollbar.bar);

    if (scrollbar.barSize > 1){
        scrollbar.barSize = 1;
    }else if (scrollbar.barSize < 0){
        scrollbar.barSize = 0;
    }

    if (scrollbar.step > 1){
        scrollbar.step = 1;
    }else if (scrollbar.step < 0){
        scrollbar.step = 0;
    }

    float barSizePixel = 0;
    float halfBar = 0;
    if (scrollbar.type == ScrollbarType::VERTICAL){
        barSizePixel = layout.height * scrollbar.barSize;
        halfBar = (barSizePixel / 2.0) / layout.height;
    }else if (scrollbar.type == ScrollbarType::HORIZONTAL){
        barSizePixel = layout.width * scrollbar.barSize;
        halfBar = (barSizePixel / 2.0) / layout.width;
    }

    if (barlayout.height != barSizePixel || barlayout.width != barSizePixel){
        barlayout.height = barSizePixel;
        barlayout.width = barSizePixel;
    }

    float pos = (scrollbar.step * ((1.0 - halfBar) - halfBar)) + halfBar;

    if (scrollbar.type == ScrollbarType::VERTICAL){
        barlayout.anchorPointLeft = 0;
        barlayout.anchorPointTop = pos;
        barlayout.anchorPointRight = 1;
        barlayout.anchorPointBottom = pos;
        barlayout.anchorOffsetLeft = 0;
        barlayout.anchorOffsetTop = -floor(barlayout.height / 2.0);
        barlayout.anchorOffsetRight = 0;
        barlayout.anchorOffsetBottom = ceil(barlayout.height / 2.0);
    }else if (scrollbar.type == ScrollbarType::HORIZONTAL){
        barlayout.anchorPointLeft = pos;
        barlayout.anchorPointTop = 0;
        barlayout.anchorPointRight = pos;
        barlayout.anchorPointBottom = 1;
        barlayout.anchorOffsetLeft = -floor(barlayout.width / 2.0);
        barlayout.anchorOffsetTop = 0;
        barlayout.anchorOffsetRight = ceil(barlayout.width / 2.0);
        barlayout.anchorOffsetBottom = 0;
    }
    barlayout.anchorPreset = AnchorPreset::NONE;
    barlayout.usingAnchors = true;
}

void UISystem::updateTextEdit(Entity entity, TextEditComponent& textedit, ImageComponent& img, UIComponent& ui, UILayoutComponent& layout){
    createTextEditObjects(entity, textedit);

    // Text
    Transform& texttransform = scene->getComponent<Transform>(textedit.text);
    UILayoutComponent& textlayout = scene->getComponent<UILayoutComponent>(textedit.text);
    UIComponent& textui = scene->getComponent<UIComponent>(textedit.text);
    TextComponent& text = scene->getComponent<TextComponent>(textedit.text);

    text.needUpdateText = true;
    createOrUpdateText(text, textui, textlayout);

    if (layout.height == 0){
        layout.height = textlayout.height + img.patchMarginTop + img.patchMarginBottom;
        layout.needUpdateSizes = true;
    }

    int heightArea = layout.height - img.patchMarginTop - img.patchMarginBottom;
    int widthArea = layout.width - img.patchMarginRight - img.patchMarginLeft - textedit.cursorWidth;

    text.multiline = false;

    int textXOffset = 0;
    if (textlayout.width > widthArea){
        textXOffset = textlayout.width - widthArea;
    }

    float textX = img.patchMarginLeft - textXOffset;
    // descend is negative
    float textY = img.patchMarginTop + (heightArea / 2) - (textlayout.height / 2);

    Vector3 textPosition = Vector3(textX, textY, 0);

    if (texttransform.position != textPosition){
        texttransform.position = textPosition;
        texttransform.needUpdate = true;
    }

    // Cursor
    Transform& cursortransform = scene->getComponent<Transform>(textedit.cursor);
    UILayoutComponent& cursorlayout = scene->getComponent<UILayoutComponent>(textedit.cursor);
    UIComponent& cursorui = scene->getComponent<UIComponent>(textedit.cursor);
    PolygonComponent& cursor = scene->getComponent<PolygonComponent>(textedit.cursor);

    createOrUpdatePolygon(cursor, cursorui, cursorlayout);

    float cursorHeight = textlayout.height;

    cursor.points.clear();
    cursor.points.push_back({Vector3(0,  0, 0),                                 Vector4(1.0, 1.0, 1.0, 1.0)});
    cursor.points.push_back({Vector3(textedit.cursorWidth, 0, 0),               Vector4(1.0, 1.0, 1.0, 1.0)});
    cursor.points.push_back({Vector3(0,  cursorHeight, 0),                      Vector4(1.0, 1.0, 1.0, 1.0)});
    cursor.points.push_back({Vector3(textedit.cursorWidth, cursorHeight, 0),    Vector4(1.0, 1.0, 1.0, 1.0)});

    float cursorX = textX + textlayout.width;
    float cursorY = img.patchMarginTop + ((float)heightArea / 2) - ((float)cursorHeight / 2);

    cursorui.color = textedit.cursorColor;
    cursortransform.position = Vector3(cursorX, cursorY, 0.0);
    cursortransform.needUpdate = true;

    cursor.needUpdatePolygon = true;
}

void UISystem::blinkCursorTextEdit(double dt, TextEditComponent& textedit, UIComponent& ui){
    textedit.cursorBlinkTimer += dt;

    Transform& cursortransform = scene->getComponent<Transform>(textedit.cursor);

    if (ui.focused){
        if (textedit.cursorBlinkTimer > 0.6) {
            cursortransform.visible = !cursortransform.visible;
            textedit.cursorBlinkTimer = 0;
        }
    }else{
        cursortransform.visible = false;
    }
}

void UISystem::createUIPolygon(PolygonComponent& polygon, UIComponent& ui, UILayoutComponent& layout){

    ui.primitiveType = PrimitiveType::TRIANGLE_STRIP;

    ui.buffer.clear();
	ui.buffer.addAttribute(AttributeType::POSITION, 3);
	ui.buffer.addAttribute(AttributeType::TEXCOORD1, 2);
    ui.buffer.addAttribute(AttributeType::COLOR, 4);
    ui.buffer.setUsage(BufferUsage::DYNAMIC);

    ui.indices.clear();

    for (int i = 0; i < polygon.points.size(); i++){
        ui.buffer.addVector3(AttributeType::POSITION, polygon.points[i].position);
        ui.buffer.addVector3(AttributeType::NORMAL, Vector3(0.0f, 0.0f, 1.0f));
        ui.buffer.addVector4(AttributeType::COLOR, polygon.points[i].color);
    }

    // Generation texcoords
    float min_X = std::numeric_limits<float>::max();
    float max_X = std::numeric_limits<float>::min();
    float min_Y = std::numeric_limits<float>::max();
    float max_Y = std::numeric_limits<float>::min();

    Attribute* attVertex = ui.buffer.getAttribute(AttributeType::POSITION);

    for (unsigned int i = 0; i < ui.buffer.getCount(); i++){
        min_X = fmin(min_X, ui.buffer.getFloat(attVertex, i, 0));
        min_Y = fmin(min_Y, ui.buffer.getFloat(attVertex, i, 1));
        max_X = fmax(max_X, ui.buffer.getFloat(attVertex, i, 0));
        max_Y = fmax(max_Y, ui.buffer.getFloat(attVertex, i, 1));
    }

    double k_X = 1/(max_X - min_X);
    double k_Y = 1/(max_Y - min_Y);

    float u = 0;
    float v = 0;

    for ( unsigned int i = 0; i < ui.buffer.getCount(); i++){
        u = (ui.buffer.getFloat(attVertex, i, 0) - min_X) * k_X;
        v = (ui.buffer.getFloat(attVertex, i, 1) - min_Y) * k_Y;

        if (ui.flipY)
            v = 1.0 - v;
        ui.buffer.addVector2(AttributeType::TEXCOORD1, Vector2(u, v));
    }

    layout.width = (int)(max_X - min_X);
    layout.height = (int)(max_Y - min_Y);

    if (ui.loaded)
        ui.needUpdateBuffer = true;
}

bool UISystem::createOrUpdatePolygon(PolygonComponent& polygon, UIComponent& ui, UILayoutComponent& layout){
    if (polygon.needUpdatePolygon){
        if (ui.automaticFlipY){
            CameraComponent& camera = scene->getComponent<CameraComponent>(scene->getCamera());
            changeFlipY(ui, camera);
        }

        createUIPolygon(polygon, ui, layout);

        polygon.needUpdatePolygon = false;
    }

    return true;
}

bool UISystem::createOrUpdateImage(ImageComponent& img, UIComponent& ui, UILayoutComponent& layout){
    if (img.needUpdatePatches){
        if (ui.automaticFlipY){
            CameraComponent& camera = scene->getComponent<CameraComponent>(scene->getCamera());
            changeFlipY(ui, camera);
        }

        createImagePatches(img, ui, layout);

        img.needUpdatePatches = false;
    }

    return true;
}

bool UISystem::createOrUpdateText(TextComponent& text, UIComponent& ui, UILayoutComponent& layout){
    if (text.needUpdateText){
        if (ui.automaticFlipY){
            CameraComponent& camera = scene->getComponent<CameraComponent>(scene->getCamera());
            changeFlipY(ui, camera);
        }

        if (text.loaded && text.needReload){
            destroyText(text);
        }
        if (!text.loaded){
            loadFontAtlas(text, ui, layout);
        }
        createText(text, ui, layout);

        text.needUpdateText = false;
    }

    return true;
}

void UISystem::applyAnchorPreset(UILayoutComponent& layout){
    if (layout.anchorPreset == AnchorPreset::TOP_LEFT){
        layout.anchorPointLeft = 0;
        layout.anchorPointTop = 0;
        layout.anchorPointRight = 0;
        layout.anchorPointBottom = 0;
        layout.anchorOffsetLeft = 0;
        layout.anchorOffsetTop = 0;
        layout.anchorOffsetRight = layout.width;
        layout.anchorOffsetBottom = layout.height;
    }else if (layout.anchorPreset == AnchorPreset::TOP_RIGHT){
        layout.anchorPointLeft = 1;
        layout.anchorPointTop = 0;
        layout.anchorPointRight = 1;
        layout.anchorPointBottom = 0;
        layout.anchorOffsetLeft = -layout.width;
        layout.anchorOffsetTop = 0;
        layout.anchorOffsetRight = 0;
        layout.anchorOffsetBottom = layout.height;
    }else if (layout.anchorPreset == AnchorPreset::BOTTOM_RIGHT){
        layout.anchorPointLeft = 1;
        layout.anchorPointTop = 1;
        layout.anchorPointRight = 1;
        layout.anchorPointBottom = 1;
        layout.anchorOffsetLeft = -layout.width;
        layout.anchorOffsetTop = -layout.height;
        layout.anchorOffsetRight = 0;
        layout.anchorOffsetBottom = 0;
    }else if (layout.anchorPreset == AnchorPreset::BOTTOM_LEFT){
        layout.anchorPointLeft = 0;
        layout.anchorPointTop = 1;
        layout.anchorPointRight = 0;
        layout.anchorPointBottom = 1;
        layout.anchorOffsetLeft = 0;
        layout.anchorOffsetTop = -layout.height;
        layout.anchorOffsetRight = layout.width;
        layout.anchorOffsetBottom = 0;
    }else if (layout.anchorPreset == AnchorPreset::CENTER_LEFT){
        layout.anchorPointLeft = 0;
        layout.anchorPointTop = 0.5;
        layout.anchorPointRight = 0;
        layout.anchorPointBottom = 0.5;
        layout.anchorOffsetLeft = 0;
        layout.anchorOffsetTop = -floor(layout.height / 2.0);
        layout.anchorOffsetRight = layout.width;
        layout.anchorOffsetBottom = ceil(layout.height / 2.0);
    }else if (layout.anchorPreset == AnchorPreset::CENTER_TOP){
        layout.anchorPointLeft = 0.5;
        layout.anchorPointTop = 0;
        layout.anchorPointRight = 0.5;
        layout.anchorPointBottom = 0;
        layout.anchorOffsetLeft = -floor(layout.width / 2.0);
        layout.anchorOffsetTop = 0;
        layout.anchorOffsetRight = ceil(layout.width / 2.0);
        layout.anchorOffsetBottom = layout.height;
    }else if (layout.anchorPreset == AnchorPreset::CENTER_RIGHT){
        layout.anchorPointLeft = 1;
        layout.anchorPointTop = 0.5;
        layout.anchorPointRight = 1;
        layout.anchorPointBottom = 0.5;
        layout.anchorOffsetLeft = -layout.width;
        layout.anchorOffsetTop = -floor(layout.height / 2.0);
        layout.anchorOffsetRight = 0;
        layout.anchorOffsetBottom = ceil(layout.height / 2.0);
    }else if (layout.anchorPreset == AnchorPreset::CENTER_BOTTOM){
        layout.anchorPointLeft = 0.5;
        layout.anchorPointTop = 1;
        layout.anchorPointRight = 0.5;
        layout.anchorPointBottom = 1;
        layout.anchorOffsetLeft = -floor(layout.width / 2.0);
        layout.anchorOffsetTop = -layout.height;
        layout.anchorOffsetRight = ceil(layout.width / 2.0);
        layout.anchorOffsetBottom = 0;
    }else if (layout.anchorPreset == AnchorPreset::CENTER){
        layout.anchorPointLeft = 0.5;
        layout.anchorPointTop = 0.5;
        layout.anchorPointRight = 0.5;
        layout.anchorPointBottom = 0.5;
        layout.anchorOffsetLeft = -floor(layout.width / 2.0);
        layout.anchorOffsetTop = -floor(layout.height / 2.0);
        layout.anchorOffsetRight = ceil(layout.width / 2.0);
        layout.anchorOffsetBottom = ceil(layout.height / 2.0);
    }else if (layout.anchorPreset == AnchorPreset::LEFT_WIDE){
        layout.anchorPointLeft = 0;
        layout.anchorPointTop = 0;
        layout.anchorPointRight = 0;
        layout.anchorPointBottom = 1;
        layout.anchorOffsetLeft = 0;
        layout.anchorOffsetTop = 0;
        layout.anchorOffsetRight = layout.width;
        layout.anchorOffsetBottom = 0;
    }else if (layout.anchorPreset == AnchorPreset::TOP_WIDE){
        layout.anchorPointLeft = 0;
        layout.anchorPointTop = 0;
        layout.anchorPointRight = 1;
        layout.anchorPointBottom = 0;
        layout.anchorOffsetLeft = 0;
        layout.anchorOffsetTop = 0;
        layout.anchorOffsetRight = 0;
        layout.anchorOffsetBottom = layout.height;
    }else if (layout.anchorPreset == AnchorPreset::RIGHT_WIDE){
        layout.anchorPointLeft = 1;
        layout.anchorPointTop = 0;
        layout.anchorPointRight = 1;
        layout.anchorPointBottom = 1;
        layout.anchorOffsetLeft = -layout.width;
        layout.anchorOffsetTop = 0;
        layout.anchorOffsetRight = 0;
        layout.anchorOffsetBottom = 0;
    }else if (layout.anchorPreset == AnchorPreset::BOTTOM_WIDE){
        layout.anchorPointLeft = 0;
        layout.anchorPointTop = 1;
        layout.anchorPointRight = 1;
        layout.anchorPointBottom = 1;
        layout.anchorOffsetLeft = 0;
        layout.anchorOffsetTop = -layout.height;
        layout.anchorOffsetRight = 0;
        layout.anchorOffsetBottom = 0;
    }else if (layout.anchorPreset == AnchorPreset::VERTICAL_CENTER_WIDE){
        layout.anchorPointLeft = 0.5;
        layout.anchorPointTop = 0;
        layout.anchorPointRight = 0.5;
        layout.anchorPointBottom = 1;
        layout.anchorOffsetLeft = -floor(layout.width / 2.0);
        layout.anchorOffsetTop = 0;
        layout.anchorOffsetRight = ceil(layout.width / 2.0);
        layout.anchorOffsetBottom = 0;
    }else if (layout.anchorPreset == AnchorPreset::HORIZONTAL_CENTER_WIDE){
        layout.anchorPointLeft = 0;
        layout.anchorPointTop = 0.5;
        layout.anchorPointRight = 1;
        layout.anchorPointBottom = 0.5;
        layout.anchorOffsetLeft = 0;
        layout.anchorOffsetTop = -floor(layout.height / 2.0);
        layout.anchorOffsetRight = 0;
        layout.anchorOffsetBottom = ceil(layout.height / 2.0);
    }else if (layout.anchorPreset == AnchorPreset::FULL_LAYOUT){
        layout.anchorPointLeft = 0;
        layout.anchorPointTop = 0;
        layout.anchorPointRight = 1;
        layout.anchorPointBottom = 1;
        layout.anchorOffsetLeft = 0;
        layout.anchorOffsetTop = 0;
        layout.anchorOffsetRight = 0;
        layout.anchorOffsetBottom = 0;
    }
}

void UISystem::changeFlipY(UIComponent& ui, CameraComponent& camera){
    ui.flipY = false;
    if (camera.type != CameraType::CAMERA_2D){
        ui.flipY = !ui.flipY;
    }

    if (ui.texture.isFramebuffer() && Engine::isOpenGL()){
        ui.flipY = !ui.flipY;
    }
}

void UISystem::destroyText(TextComponent& text){
    text.loaded = false;
    text.needReload = false;

    text.needUpdateText = true;

    if (text.stbtext){
        text.stbtext.reset();
        text.stbtext = NULL;
    }
}

void UISystem::destroyButton(ButtonComponent& button){
    button.textureNormal.destroy();
    button.texturePressed.destroy();
    button.textureDisabled.destroy();

    button.needUpdateButton = true;

    if (button.label != NULL_ENTITY){
        scene->destroyEntity(button.label);
        button.label = NULL_ENTITY;
    }
}

void UISystem::destroyPanel(PanelComponent& panel){
    panel.needUpdatePanel = true;

    if (panel.headertext != NULL_ENTITY){
        scene->destroyEntity(panel.headertext);
        panel.headertext = NULL_ENTITY;
    }
    if (panel.headercontainer != NULL_ENTITY){
        scene->destroyEntity(panel.headercontainer);
        panel.headercontainer = NULL_ENTITY;
    }
    if (panel.headerimage != NULL_ENTITY){
        scene->destroyEntity(panel.headerimage);
        panel.headerimage = NULL_ENTITY;
    }
}

void UISystem::destroyScrollbar(ScrollbarComponent& scrollbar){
    scrollbar.needUpdateScrollbar = true;

    if (scrollbar.bar != NULL_ENTITY){
        scene->destroyEntity(scrollbar.bar);
        scrollbar.bar = NULL_ENTITY;
    }
}

void UISystem::destroyTextEdit(TextEditComponent& textedit){
    textedit.needUpdateTextEdit = true;

    if (textedit.text != NULL_ENTITY){
        scene->destroyEntity(textedit.text);
        textedit.text = NULL_ENTITY;
    }
    if (textedit.cursor != NULL_ENTITY){
        scene->destroyEntity(textedit.cursor);
        textedit.cursor = NULL_ENTITY;
    }
}

void UISystem::load(){
    update(0);
}

void UISystem::destroy(){
    auto layouts = scene->getComponentArray<UILayoutComponent>();
    for (int i = 0; i < layouts->size(); i++) {
        UILayoutComponent& layout = layouts->getComponentFromIndex(i);
        Entity entity = layouts->getEntity(i);
        Signature signature = scene->getSignature(entity);

        if (signature.test(scene->getComponentType<TextComponent>())) {
            TextComponent &text = scene->getComponent<TextComponent>(entity);

            destroyText(text);
        }
        if (signature.test(scene->getComponentType<ButtonComponent>())) {
            ButtonComponent &button = scene->getComponent<ButtonComponent>(entity);

            destroyButton(button);
        }
        if (signature.test(scene->getComponentType<PanelComponent>())){
            PanelComponent& panel = scene->getComponent<PanelComponent>(entity);

            destroyPanel(panel);
        }
        if (signature.test(scene->getComponentType<ScrollbarComponent>())){
            ScrollbarComponent& scrollbar = scene->getComponent<ScrollbarComponent>(entity);

            destroyScrollbar(scrollbar);
        }
        if (signature.test(scene->getComponentType<TextEditComponent>())) {
            TextEditComponent &textedit = scene->getComponent<TextEditComponent>(entity);

            destroyTextEdit(textedit);
        }
    }
}

void UISystem::draw(){
}

void UISystem::createOrUpdateUiComponent(double dt, UILayoutComponent& layout, Entity entity, Signature signature){
    if (signature.test(scene->getComponentType<UIComponent>())){
        UIComponent& ui = scene->getComponent<UIComponent>(entity);

        // Texts
        if (signature.test(scene->getComponentType<TextComponent>())){
            TextComponent& text = scene->getComponent<TextComponent>(entity);

            createOrUpdateText(text, ui, layout);
        }

        // UI Polygons
        if (signature.test(scene->getComponentType<PolygonComponent>())){
            PolygonComponent& polygon = scene->getComponent<PolygonComponent>(entity);

            createOrUpdatePolygon(polygon, ui, layout);
        }

        // Images
        if (signature.test(scene->getComponentType<ImageComponent>())){
            ImageComponent& img = scene->getComponent<ImageComponent>(entity);

            createOrUpdateImage(img, ui, layout);

            // Buttons
            if (signature.test(scene->getComponentType<ButtonComponent>())){
                ButtonComponent& button = scene->getComponent<ButtonComponent>(entity);

                if (button.needUpdateButton){
                    updateButton(entity, button, img, ui, layout);

                    button.needUpdateButton = false;
                }
            }

            // Panels
            if (signature.test(scene->getComponentType<PanelComponent>())){
                PanelComponent& panel = scene->getComponent<PanelComponent>(entity);

                if (panel.needUpdatePanel){
                    updatePanel(entity, panel, img, ui, layout);

                    panel.needUpdatePanel = false;
                }
            }

            // Scrollbar
            if (signature.test(scene->getComponentType<ScrollbarComponent>())){
                ScrollbarComponent& scrollbar = scene->getComponent<ScrollbarComponent>(entity);

                if (scrollbar.needUpdateScrollbar){
                    updateScrollbar(entity, scrollbar, img, ui, layout);

                    scrollbar.needUpdateScrollbar = false;
                }
            }

            // Textedits
            if (signature.test(scene->getComponentType<TextEditComponent>())){
                TextEditComponent& textedit = scene->getComponent<TextEditComponent>(entity);

                if (textedit.needUpdateTextEdit){
                    updateTextEdit(entity, textedit, img, ui, layout);

                    textedit.needUpdateTextEdit = false;
                }

                blinkCursorTextEdit(dt, textedit, ui);
            }
        }
    }
}

void UISystem::update(double dt){

    // need to be ordered by Transform
    auto layouts = scene->getComponentArray<UILayoutComponent>();

    for (int i = 0; i < layouts->size(); i++){
        UILayoutComponent& layout = layouts->getComponentFromIndex(i);
        Entity entity = layouts->getEntity(i);
        Signature signature = scene->getSignature(entity);

        if (signature.test(scene->getComponentType<UIContainerComponent>())){
            UIContainerComponent& container = scene->getComponent<UIContainerComponent>(entity);
            // reseting all container boxes
            for (int b = 0; b < container.numBoxes; b++){
                container.boxes[b].layout = NULL_ENTITY;
            }
            container.numBoxes = 0;
        }

        if (signature.test(scene->getComponentType<Transform>())){
            Transform& transform = scene->getComponent<Transform>(entity);
            UILayoutComponent* parentlayout = scene->findComponent<UILayoutComponent>(transform.parent);
            if (parentlayout){
                UIContainerComponent* parentcontainer = scene->findComponent<UIContainerComponent>(transform.parent);
                if (parentcontainer && transform.visible){
                    if (parentcontainer->numBoxes < MAX_CONTAINER_BOXES){
                        layout.containerBoxIndex = parentcontainer->numBoxes;
                        if (!layout.usingAnchors){
                            layout.anchorPreset = AnchorPreset::TOP_LEFT;
                            layout.usingAnchors = true;
                        }
                        parentcontainer->boxes[layout.containerBoxIndex].layout = entity;

                        parentcontainer->numBoxes = parentcontainer->numBoxes + 1;
                    }else{
                        transform.parent = NULL_ENTITY;
                        Log::error("The UI container has exceeded the maximum allowed of %i children. Please, increase MAX_CONTAINER_BOXES value.", MAX_CONTAINER_BOXES);
                    }
                }
            }
        }
    }

    // reverse Transform order to get sizes
    for (int i = layouts->size()-1; i >= 0; i--){
        UILayoutComponent& layout = layouts->getComponentFromIndex(i);
        Entity entity = layouts->getEntity(i);
        Signature signature = scene->getSignature(entity);

        createOrUpdateUiComponent(dt, layout, entity, signature);

        if (signature.test(scene->getComponentType<UIContainerComponent>())){
            UIContainerComponent& container = scene->getComponent<UIContainerComponent>(entity);
            // configuring all container boxes
            if (container.numBoxes > 0){

                container.fixedWidth = 0;
                container.fixedHeight = 0;
                container.numBoxExpand = 0;
                container.maxWidth = 0;
                container.maxHeight = 0;
                int totalWidth = 0;
                int totalHeight = 0;
                for (int b = 0; b < container.numBoxes; b++){
                    if (container.boxes[b].layout != NULL_ENTITY){
                        if (!container.boxes[b].expand){
                            container.fixedWidth += container.boxes[b].rect.getWidth();
                        }
                        if (!container.boxes[b].expand){
                            container.fixedHeight += container.boxes[b].rect.getHeight();
                        }
                        totalWidth += container.boxes[b].rect.getWidth();
                        totalHeight += container.boxes[b].rect.getHeight();
                        container.maxHeight = std::max(container.maxHeight, (int)container.boxes[b].rect.getHeight());
                        container.maxWidth = std::max(container.maxWidth, (int)container.boxes[b].rect.getWidth());
                    }
                    if (container.boxes[b].expand){
                        container.numBoxExpand++;
                    }
                }


                if (container.type == ContainerType::HORIZONTAL){
                    layout.width = (layout.width > 0)? layout.width : totalWidth;
                    layout.height = (layout.height > 0)? layout.height : container.maxHeight;
                }else if (container.type == ContainerType::VERTICAL){
                    layout.width = (layout.width > 0)? layout.width : container.maxWidth;
                    layout.height = (layout.height > 0)? layout.height : totalHeight;
                }else if (container.type == ContainerType::FLOAT){
                    layout.width = (layout.width > 0)? layout.width : totalWidth;
                    layout.height = (layout.height > 0)? layout.height : container.maxHeight;
                }
            }
        }

        if (signature.test(scene->getComponentType<Transform>())){
            Transform& transform = scene->getComponent<Transform>(entity);
            UIContainerComponent* parentcontainer = scene->findComponent<UIContainerComponent>(transform.parent);
            if (parentcontainer && layout.containerBoxIndex >= 0){
                parentcontainer->boxes[layout.containerBoxIndex].rect = Rect(0, 0, layout.width, layout.height);
            }
        }
    }

    for (int i = 0; i < layouts->size(); i++){
        UILayoutComponent& layout = layouts->getComponentFromIndex(i);
        Entity entity = layouts->getEntity(i);
        Signature signature = scene->getSignature(entity);

        if (signature.test(scene->getComponentType<Transform>())){
            Transform& transform = scene->getComponent<Transform>(entity);

            if (layout.usingAnchors){
                if (layout.anchorPointRight < layout.anchorPointLeft)
                    layout.anchorPointRight = layout.anchorPointLeft;
                if (layout.anchorPointBottom < layout.anchorPointTop)
                    layout.anchorPointBottom = layout.anchorPointTop;

                applyAnchorPreset(layout);
            }

            float abAnchorLeft = 0;
            float abAnchorRight = 0;
            float abAnchorTop = 0;
            float abAnchorBottom = 0;
            if (transform.parent == NULL_ENTITY){
                abAnchorLeft = Engine::getCanvasWidth() * layout.anchorPointLeft;
                abAnchorRight = Engine::getCanvasWidth() * layout.anchorPointRight;
                abAnchorTop = Engine::getCanvasHeight() * layout.anchorPointTop;
                abAnchorBottom = Engine::getCanvasHeight() * layout.anchorPointBottom;
            }else{
                UILayoutComponent* parentlayout = scene->findComponent<UILayoutComponent>(transform.parent);
                if (parentlayout){
                    if (parentlayout->ignoreScissor){
                        layout.ignoreScissor = true;
                    }

                    Rect boxRect = Rect(0, 0, parentlayout->width, parentlayout->height);

                    UIContainerComponent* parentcontainer = scene->findComponent<UIContainerComponent>(transform.parent);
                    if (parentcontainer){
                        boxRect = parentcontainer->boxes[layout.containerBoxIndex].rect;
                    }

                    ImageComponent* parentimage = scene->findComponent<ImageComponent>(transform.parent);
                    if (parentimage){
                        if (!layout.ignoreScissor){
                            boxRect.setX(boxRect.getX() + parentimage->patchMarginLeft);
                            boxRect.setWidth(boxRect.getWidth() - parentimage->patchMarginRight - parentimage->patchMarginLeft);
                            boxRect.setY(boxRect.getY() + parentimage->patchMarginTop);
                            boxRect.setHeight(boxRect.getHeight() - parentimage->patchMarginBottom - parentimage->patchMarginTop);
                        }else{
                            boxRect.setX(boxRect.getX());
                            boxRect.setWidth(boxRect.getWidth());
                            boxRect.setY(boxRect.getY());
                            boxRect.setHeight(boxRect.getHeight());
                        }
                    }

                    abAnchorLeft = (boxRect.getWidth() * layout.anchorPointLeft) + boxRect.getX();
                    abAnchorRight = (boxRect.getWidth() * layout.anchorPointRight) + boxRect.getX();
                    abAnchorTop = (boxRect.getHeight() * layout.anchorPointTop) + boxRect.getY();
                    abAnchorBottom = (boxRect.getHeight() * layout.anchorPointBottom) + boxRect.getY();
                }
            }

            if (layout.usingAnchors){

                float posX = abAnchorLeft + layout.anchorOffsetLeft;
                float posY = abAnchorTop + layout.anchorOffsetTop;

                if (posX != transform.position.x || posY != transform.position.y){
                    transform.position.x = posX;
                    transform.position.y = posY;
                    transform.needUpdate = true;
                }

                float width = abAnchorRight - transform.position.x + layout.anchorOffsetRight;
                float height = abAnchorBottom - transform.position.y + layout.anchorOffsetBottom;

                if (width != layout.width || height != layout.height){
                    layout.width = width;
                    layout.height = height;
                    layout.needUpdateSizes = true;
                }

            }else{
                layout.anchorOffsetLeft = transform.position.x - abAnchorLeft;
                layout.anchorOffsetTop = transform.position.y - abAnchorTop;
                layout.anchorOffsetRight = layout.width + transform.position.x - abAnchorRight;
                layout.anchorOffsetBottom = layout.height + transform.position.y - abAnchorBottom;
            }
        }

        if (signature.test(scene->getComponentType<UIContainerComponent>())){
            UIContainerComponent& container = scene->getComponent<UIContainerComponent>(entity);
            // configuring all container boxes
            if (container.numBoxes > 0){

                int usedSize = 0;
                for (int b = 0; b < container.numBoxes; b++){
                    if (container.boxes[b].layout != NULL_ENTITY){
                        if (container.type == ContainerType::HORIZONTAL){
                            if (b > 0){
                                container.boxes[b].rect.setX(container.boxes[b-1].rect.getX() + container.boxes[b-1].rect.getWidth());
                            }
                            if (container.boxes[b].rect.getWidth() >= layout.width){
                                container.boxes[b].rect.setWidth(layout.width / container.numBoxes);
                            }
                            if (container.boxes[b].expand){
                                float diff = layout.width - container.fixedWidth;
                                float sizeInc = (diff / container.numBoxExpand) - usedSize;
                                if (sizeInc >= container.boxes[b].rect.getWidth()) {
                                    container.boxes[b].rect.setWidth(sizeInc);
                                    usedSize = 0;
                                }else{
                                    usedSize += container.boxes[b].rect.getWidth();
                                }
                            }
                            container.boxes[b].rect.setHeight(layout.height);
                        }else if (container.type == ContainerType::VERTICAL){
                            if (b > 0){
                                container.boxes[b].rect.setY(container.boxes[b-1].rect.getY() + container.boxes[b-1].rect.getHeight());
                            }
                            if (container.boxes[b].rect.getHeight() >= layout.height){
                                container.boxes[b].rect.setHeight(layout.height / container.numBoxes);
                            }
                            if (container.boxes[b].expand){
                                float diff = layout.height - container.fixedHeight;
                                float sizeInc = (diff / container.numBoxExpand) - usedSize;
                                if (sizeInc >= container.boxes[b].rect.getHeight()) {
                                    container.boxes[b].rect.setHeight(sizeInc);
                                    usedSize = 0;
                                }else{
                                    usedSize += container.boxes[b].rect.getHeight();
                                }
                            }
                            container.boxes[b].rect.setWidth(layout.width);
                        }else if (container.type == ContainerType::FLOAT){
                            if (b > 0){
                                container.boxes[b].rect.setX(container.boxes[b-1].rect.getX() + container.boxes[b-1].rect.getWidth());
                                container.boxes[b].rect.setY(container.boxes[b-1].rect.getY());
                            }
                            if (container.boxes[b].expand){
                                int numObjInLine = floor((float)layout.width / (float)container.maxWidth);
                                float diff = layout.width - (numObjInLine * container.maxWidth);
                                container.boxes[b].rect.setWidth(container.maxWidth + (diff / numObjInLine));
                            }
                            if ((container.boxes[b].rect.getX()+container.boxes[b].rect.getWidth()) > layout.width){
                                container.boxes[b].rect.setX(0);
                                container.boxes[b].rect.setY(container.boxes[b-1].rect.getY() + container.maxHeight);
                            }
                            container.boxes[b].rect.setHeight(container.maxHeight);
                        }
                    }
                }
            }
        }

        if (layout.needUpdateSizes){
            if (signature.test(scene->getComponentType<ImageComponent>())){
                ImageComponent& img = scene->getComponent<ImageComponent>(entity);

                img.needUpdatePatches = true;
            }

            if (signature.test(scene->getComponentType<TextComponent>())){
                TextComponent& text = scene->getComponent<TextComponent>(entity);

                text.needUpdateText = true;
            }

            if (signature.test(scene->getComponentType<ScrollbarComponent>())){
                ScrollbarComponent& scrollbar = scene->getComponent<ScrollbarComponent>(entity);

                scrollbar.needUpdateScrollbar = true;
            }

            layout.needUpdateSizes = false;
        }

        createOrUpdateUiComponent(dt, layout, entity, signature);
        
    }

}

void UISystem::entityDestroyed(Entity entity){
    Signature signature = scene->getSignature(entity);

    if (signature.test(scene->getComponentType<TextComponent>())){
        TextComponent& text = scene->getComponent<TextComponent>(entity);

        destroyText(text);
    }
    if (signature.test(scene->getComponentType<ButtonComponent>())){
        ButtonComponent& button = scene->getComponent<ButtonComponent>(entity);

        destroyButton(button);
    }
    if (signature.test(scene->getComponentType<PanelComponent>())){
        PanelComponent& panel = scene->getComponent<PanelComponent>(entity);

        destroyPanel(panel);
    }
    if (signature.test(scene->getComponentType<ScrollbarComponent>())){
        ScrollbarComponent& scrollbar = scene->getComponent<ScrollbarComponent>(entity);

        destroyScrollbar(scrollbar);
    }
    if (signature.test(scene->getComponentType<TextEditComponent>())){
        TextEditComponent& textedit = scene->getComponent<TextEditComponent>(entity);

        destroyTextEdit(textedit);
    }
}

void UISystem::eventOnCharInput(wchar_t codepoint){
    auto layouts = scene->getComponentArray<UILayoutComponent>();
    for (int i = 0; i < layouts->size(); i++){
        UILayoutComponent& layout = layouts->getComponentFromIndex(i);

        Entity entity = layouts->getEntity(i);
        Signature signature = scene->getSignature(entity);
        if (signature.test(scene->getComponentType<TextEditComponent>()) && signature.test(scene->getComponentType<UIComponent>())){
            TextEditComponent& textedit = scene->getComponent<TextEditComponent>(entity);
            UIComponent& ui = scene->getComponent<UIComponent>(entity);

            if (ui.focused){
                TextComponent& text = scene->getComponent<TextComponent>(textedit.text);
                if (codepoint == '\b'){
                    if (text.text.length() > 0){
                        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > convert;
                        std::wstring utf16OldText = convert.from_bytes(text.text);

                        text.text = convert.to_bytes(utf16OldText.substr(0, utf16OldText.size() - 1));
                    }
                }else{
                    text.text = text.text + StringUtils::toUTF8(codepoint);
                }
                text.needUpdateText = true;

                textedit.needUpdateTextEdit = true;
            }
        }
    }
}

bool UISystem::eventOnPointerDown(float x, float y){
    lastUIFromPointer = NULL_ENTITY;
    lastPanelFromPointer = NULL_ENTITY;
    lastPointerPos = Vector2(x, y);

    auto layouts = scene->getComponentArray<UILayoutComponent>();

    for (int i = 0; i < layouts->size(); i++){
        UILayoutComponent& layout = layouts->getComponentFromIndex(i);

        Entity entity = layouts->getEntity(i);
        Signature signature = scene->getSignature(entity);
        if (signature.test(scene->getComponentType<Transform>()) && signature.test(scene->getComponentType<UIComponent>())){
            Transform& transform = scene->getComponent<Transform>(entity);
            UIComponent& ui = scene->getComponent<UIComponent>(entity);

            if (isCoordInside(x, y, transform, layout) && !layout.ignoreEvents){ //TODO: isCoordInside to polygon
                if (signature.test(scene->getComponentType<ImageComponent>())){
                    lastUIFromPointer = entity;
                }
                if (signature.test(scene->getComponentType<PanelComponent>())){
                    lastPanelFromPointer = entity;
                }
            }

            if (ui.focused){
                ui.focused = false;
                ui.onLostFocus.call();
            }
        }
    }

    if (lastUIFromPointer != NULL_ENTITY){
        UILayoutComponent& layout = layouts->getComponentFromIndex(layouts->getIndex(lastUIFromPointer));
        Signature signature = scene->getSignature(lastUIFromPointer);

        if (signature.test(scene->getComponentType<Transform>()) && signature.test(scene->getComponentType<UIComponent>())){
            Transform& transform = scene->getComponent<Transform>(lastUIFromPointer);
            UIComponent& ui = scene->getComponent<UIComponent>(lastUIFromPointer);
            
            if (signature.test(scene->getComponentType<ButtonComponent>())){
                ButtonComponent& button = scene->getComponent<ButtonComponent>(lastUIFromPointer);
                if (!button.disabled && !button.pressed){
                    ui.texture = button.texturePressed;
                    ui.needUpdateTexture = true;
                    button.onPress.call();
                    button.pressed = true;
                }
            }

            if (signature.test(scene->getComponentType<TextEditComponent>())){
                TextEditComponent& textedit = scene->getComponent<TextEditComponent>(lastUIFromPointer);
                TextComponent& text = scene->getComponent<TextComponent>(textedit.text);

                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
                std::wstring utf16Text = convert.from_bytes(text.text);

                System::instance().showVirtualKeyboard(utf16Text);
            }else{
                System::instance().hideVirtualKeyboard();
            }

            if (signature.test(scene->getComponentType<ScrollbarComponent>())){
                ScrollbarComponent& scrollbar = scene->getComponent<ScrollbarComponent>(lastUIFromPointer);
                Transform& bartransform = scene->getComponent<Transform>(scrollbar.bar);
                UILayoutComponent& barlayout = scene->getComponent<UILayoutComponent>(scrollbar.bar);

                if (isCoordInside(x, y, bartransform, barlayout)){
                    scrollbar.barPointerDown = true;
                    if (scrollbar.type == ScrollbarType::VERTICAL){
                        scrollbar.barPointerPos = y - transform.worldPosition.y - (bartransform.position.y * bartransform.worldScale.y);
                    }else if (scrollbar.type == ScrollbarType::HORIZONTAL){
                        scrollbar.barPointerPos = x - transform.worldPosition.x - (bartransform.position.x * bartransform.worldScale.x);
                    }
                }
            }

            if (signature.test(scene->getComponentType<PanelComponent>())){
                PanelComponent& panel = scene->getComponent<PanelComponent>(lastUIFromPointer);
                Transform& headertransform = scene->getComponent<Transform>(panel.headercontainer);
                UILayoutComponent& headerlayout = scene->getComponent<UILayoutComponent>(panel.headercontainer);

                int minX;
                int minY;
                int maxX;
                int maxY;

                AABB edgeRight;
                AABB edgeRightBottom;
                AABB edgeBottom;
                AABB edgeLeftBottom;
                AABB edgeLeft;

                Vector2 scaledSize = Vector2(layout.width * transform.worldScale.x, layout.height * transform.worldScale.y);
                Vector2 scaledResizeSize = Vector2(layout.resizeMargin * transform.worldScale.x, layout.resizeMargin * transform.worldScale.y);
                float scaledHeaderHeight = headerlayout.height * transform.worldScale.y;

                // right
                minX = transform.worldPosition.x + scaledSize.x - scaledResizeSize.x;
                minY = transform.worldPosition.y + scaledHeaderHeight;
                maxX = transform.worldPosition.x + scaledSize.x;
                maxY = transform.worldPosition.y + scaledSize.y - scaledResizeSize.y - 1;
                edgeRight = AABB(minX, minY, 0, maxX, maxY, 0);

                // right-bottom
                minX = transform.worldPosition.x + scaledSize.x - scaledResizeSize.x;
                minY = transform.worldPosition.y + scaledSize.y - scaledResizeSize.y;
                maxX = transform.worldPosition.x + scaledSize.x;
                maxY = transform.worldPosition.y + scaledSize.y;
                edgeRightBottom = AABB(minX, minY, 0, maxX, maxY, 0);

                // bottom
                minX = transform.worldPosition.x + scaledResizeSize.x + 1;
                minY = transform.worldPosition.y + scaledSize.y - scaledResizeSize.y;
                maxX = transform.worldPosition.x + scaledSize.x - scaledResizeSize.x - 1;
                maxY = transform.worldPosition.y + scaledSize.y;
                edgeBottom = AABB(minX, minY, 0, maxX, maxY, 0);

                // left-bottom
                minX = transform.worldPosition.x;
                minY = transform.worldPosition.y + scaledSize.y - scaledResizeSize.y;
                maxX = transform.worldPosition.x + scaledResizeSize.x;
                maxY = transform.worldPosition.y + scaledSize.y;
                edgeLeftBottom = AABB(minX, minY, 0, maxX, maxY, 0);

                // left
                minX = transform.worldPosition.x;
                minY = transform.worldPosition.y + scaledHeaderHeight;
                maxX = transform.worldPosition.x + scaledResizeSize.x;
                maxY = transform.worldPosition.y + scaledSize.y - scaledResizeSize.y - 1;
                edgeLeft = AABB(minX, minY, 0, maxX, maxY, 0);

                if (edgeRight.contains(Vector3(x, y, 0))){
                    //printf("Right\n");
                }

                if (edgeRightBottom.contains(Vector3(x, y, 0))){
                    //printf("RightBottom\n");
                }

                if (edgeBottom.contains(Vector3(x, y, 0))){
                    //printf("Bottom\n");
                }

                if (edgeLeftBottom.contains(Vector3(x, y, 0))){
                    //printf("LeftBottom\n");
                }

                if (edgeLeft.contains(Vector3(x, y, 0))){
                    //printf("Left\n");
                }

                if (panel.canMove){
                    if (isCoordInside(x, y, headertransform, headerlayout)){
                        panel.headerPointerDown = true;
                    }
                }

                if (panel.canTopOnFocus){
                    scene->moveChildToTop(lastUIFromPointer);
                }
            }

            ui.onPointerDown(x - transform.worldPosition.x, y - transform.worldPosition.y);

            if (!ui.focused){
                ui.focused = true;
                ui.onGetFocus.call();
            }
        }
    }else{
        System::instance().hideVirtualKeyboard();
    }

    if (lastPanelFromPointer != NULL_ENTITY){
        PanelComponent& panel = scene->getComponent<PanelComponent>(lastPanelFromPointer);

        if (panel.canTopOnFocus){
            scene->moveChildToTop(lastPanelFromPointer);
        }
    }

    if (lastUIFromPointer != NULL_ENTITY)
        return true;

    return false;
}

bool UISystem::eventOnPointerUp(float x, float y){
    lastPointerPos = Vector2(-1, -1);

    auto layouts = scene->getComponentArray<UILayoutComponent>();
    for (int i = 0; i < layouts->size(); i++){
        UILayoutComponent& layout = layouts->getComponentFromIndex(i);

        Entity entity = layouts->getEntity(i);
        Signature signature = scene->getSignature(entity);
        if (signature.test(scene->getComponentType<Transform>()) && signature.test(scene->getComponentType<UIComponent>())){
            Transform& transform = scene->getComponent<Transform>(entity);
            UIComponent& ui = scene->getComponent<UIComponent>(entity);

            if (signature.test(scene->getComponentType<ButtonComponent>())){
                ButtonComponent& button = scene->getComponent<ButtonComponent>(entity);
                if (!button.disabled && button.pressed){
                    ui.texture = button.textureNormal;
                    ui.needUpdateTexture = true;
                    button.pressed = false;
                    button.onRelease.call();
                }
            }

            if (signature.test(scene->getComponentType<ScrollbarComponent>())){
                ScrollbarComponent& scrollbar = scene->getComponent<ScrollbarComponent>(entity);
                Transform& bartransform = scene->getComponent<Transform>(scrollbar.bar);
                UILayoutComponent& barlayout = scene->getComponent<UILayoutComponent>(scrollbar.bar);

                if (isCoordInside(x, y, bartransform, barlayout)){
                    scrollbar.barPointerDown = false;
                }
            }

            if (signature.test(scene->getComponentType<PanelComponent>())){
                PanelComponent& panel = scene->getComponent<PanelComponent>(entity);
                Transform& headertransform = scene->getComponent<Transform>(panel.headercontainer);
                UILayoutComponent& headerlayout = scene->getComponent<UILayoutComponent>(panel.headercontainer);

                if (isCoordInside(x, y, headertransform, headerlayout)){
                    panel.headerPointerDown = false;
                }
            }

            ui.onPointerUp(x - transform.worldPosition.x, y - transform.worldPosition.y);
        }
    }

    lastPanelFromPointer = NULL_ENTITY;

    if (lastUIFromPointer != NULL_ENTITY){
        lastUIFromPointer = NULL_ENTITY;
        return true;
    }

    return false;
}

bool UISystem::eventOnPointerMove(float x, float y){
    auto layouts = scene->getComponentArray<UILayoutComponent>();

    if (lastUIFromPointer != NULL_ENTITY){
        UILayoutComponent& layout = layouts->getComponentFromIndex(layouts->getIndex(lastUIFromPointer));
        Signature signature = scene->getSignature(lastUIFromPointer);

        if (signature.test(scene->getComponentType<Transform>()) && signature.test(scene->getComponentType<UIComponent>())){
            Transform& transform = scene->getComponent<Transform>(lastUIFromPointer);
            UIComponent& ui = scene->getComponent<UIComponent>(lastUIFromPointer);

            ui.onPointerMove.call(x - transform.worldPosition.x, y - transform.worldPosition.y);
            ui.pointerMoved = true;
        }

        if (signature.test(scene->getComponentType<ScrollbarComponent>())){
            ScrollbarComponent& scrollbar = scene->getComponent<ScrollbarComponent>(lastUIFromPointer);
            Transform& transform = scene->getComponent<Transform>(lastUIFromPointer);
            Transform& bartransform = scene->getComponent<Transform>(scrollbar.bar);
            UILayoutComponent& barlayout = scene->getComponent<UILayoutComponent>(scrollbar.bar);

            if (scrollbar.barPointerDown){

                float pos = 0;
                float halfBar = 0;

                if (scrollbar.type == ScrollbarType::VERTICAL){
                    float barSizePixel = (layout.height * scrollbar.barSize) * transform.worldScale.y;
                    pos = (y - transform.worldPosition.y + ((barSizePixel / 2.0) - scrollbar.barPointerPos)) / (layout.height * transform.worldScale.y);
                    halfBar = (barSizePixel / 2.0) / (layout.height * transform.worldScale.y);
                }else if (scrollbar.type == ScrollbarType::HORIZONTAL){
                    float barSizePixel = (layout.width * scrollbar.barSize) * transform.worldScale.x;
                    pos = (x - transform.worldPosition.x + ((barSizePixel / 2.0) - scrollbar.barPointerPos)) / (layout.width * transform.worldScale.x);
                    halfBar = (barSizePixel / 2.0) / (layout.width * transform.worldScale.x);
                }

                if (pos < halfBar){
                    pos = halfBar;
                }else if (pos > (1.0 - halfBar)){
                    pos = (1.0 - halfBar);
                }

                float newStep = (pos - halfBar) / ((1.0 - halfBar) - halfBar);

                if (newStep != scrollbar.step){
                    scrollbar.step = newStep;
                    scrollbar.onChange.call();
                }

                if (scrollbar.type == ScrollbarType::VERTICAL){
                    barlayout.anchorPointTop = pos;
                    barlayout.anchorPointBottom = pos;
                }else if (scrollbar.type == ScrollbarType::HORIZONTAL){
                    barlayout.anchorPointLeft = pos;
                    barlayout.anchorPointRight = pos;
                }
            }
        }

        if (signature.test(scene->getComponentType<PanelComponent>())){
            PanelComponent& panel = scene->getComponent<PanelComponent>(lastUIFromPointer);
            Transform& transform = scene->getComponent<Transform>(lastUIFromPointer);
            if (panel.headerPointerDown){
                Vector2 movement = Vector2(x, y) - lastPointerPos;
                transform.position += Vector3(movement.x / transform.worldScale.x, movement.y / transform.worldScale.y, 0);
                transform.needUpdate = true;
            }
        }
    }

    lastPointerPos = Vector2(x, y);

    if (lastUIFromPointer != NULL_ENTITY)
        return true;

    return false;
}

bool UISystem::isCoordInside(float x, float y, Transform& transform, UILayoutComponent& layout){
    Vector3 point = transform.worldRotation.getRotationMatrix() * Vector3(x, y, 0);

    if (point.x >= (transform.worldPosition.x) &&
        point.x <= (transform.worldPosition.x + abs(layout.width * transform.worldScale.x)) &&
        point.y >= (transform.worldPosition.y) &&
        point.y <= (transform.worldPosition.y + abs(layout.height * transform.worldScale.y))) {
        return true;
    }
    return false;
}

bool UISystem::isCoordInside(float x, float y, Transform& transform, UILayoutComponent& layout, Vector2 center){
    Vector3 point = transform.worldRotation.getRotationMatrix() * Vector3(x, y, 0);

    if (point.x >= (transform.worldPosition.x - center.x) &&
        point.x <= (transform.worldPosition.x - center.x + abs(layout.width * transform.worldScale.x)) &&
        point.y >= (transform.worldPosition.y - center.y) &&
        point.y <= (transform.worldPosition.y - center.y + abs(layout.height * transform.worldScale.y))) {
        return true;
    }
    return false;
}
