#pragma once
#include "djui.h"

struct DjuiFlowLayout {
    struct DjuiBase base;
    enum DjuiFlowDirection flowDirection;
    struct DjuiScreenValue margin;
#if defined(__XSDK__)
    f32 scrollOffset;
#endif
};

void djui_flow_layout_set_flow_direction(struct DjuiFlowLayout* layout, enum DjuiFlowDirection flowDirection);
void djui_flow_layout_set_margin(struct DjuiFlowLayout* layout, f32 margin);
void djui_flow_layout_set_margin_type(struct DjuiFlowLayout* layout, enum DjuiScreenValueType marginType);

#if defined(__XSDK__)
bool djui_flow_layout_render(struct DjuiBase* base);
#endif
struct DjuiFlowLayout* djui_flow_layout_create(struct DjuiBase* parent);
