/* pl_notification.c — pl.Notification：Toast 通知条（库级 build-widget）。
 *
 * visible=true 时通过 Portal z=300 显示在视口右上角（或底部中央）。
 * 动画：pl_anim_f 驱动 opacity 0→1 淡入；duration 秒后调 on_close 触发消失。
 * 计时：用 pl_phase（秒周期）+ 内部 state 记录显示起始相位，差值≥duration 时触发。
 * visible=false 时不渲染任何内容。
 *
 * 颜色方案：
 *   Info    — indigo 背景
 *   Success — green 背景
 *   Warning — amber 背景（深色文字）
 *   Error   — red 背景
 * 位置：视口右上角 (right=20, top=20)，用 Portal OverlayX/Y 绝对定位。
 */
#include "pl_notification.h"
#include "theme/pl_theme.h"
#include <plume/plume_kernel.h>
#include <plume/plume_state.h>
#include <plume/plume_ui.h>
#include <math.h>

/* variant → (bg, text) */
static void notify_colors(int variant, PlColor* bg, PlColor* tc){
    switch(variant){
        case kPlNotify_Success: *bg=0x166534FFu; *tc=0xFFFFFFFFu; break;
        case kPlNotify_Warning: *bg=0x92400EFFu; *tc=0xFFFFFFFFu; break;
        case kPlNotify_Error:   *bg=0x991B1BFFu; *tc=0xFFFFFFFFu; break;
        default:                *bg=0x1E3A5FFFu; *tc=0xFFFFFFFFu; break;
    }
}

typedef struct { int kind; float x,y,dx,dy; unsigned key; const char* text; unsigned mods; int button; } PlNFire_;
static void notify_close(PlCtx ctx, PlEvent ev){
    (void)ev;
    PlHandler cb=(PlHandler)pl_ptr(pl_state_ptr(ctx,"_ncb",NULL));
    if(cb) cb(ctx,ev);
}

static PlNode notify_build(PlCtx ctx, PlProps props){
    bool visible    = pl_props_bool (props,kPlPropNotification_Visible, false);
    const char* msg = pl_props_str  (props,kPlPropNotification_Message, "");
    float dur       = pl_props_f    (props,kPlPropNotification_Duration,0.0f);
    int variant     = pl_props_i    (props,kPlPropNotification_Variant, 0);
    PlHandler cb    = (PlHandler)pl_props_ptr(props,kPlPropNotification_OnClose,NULL);

    pl_set_ptr(pl_state_ptr(ctx,"_ncb",NULL),(void*)cb);

    /* opacity 动画：visible=true → 1.0, false → 0.0 */
    float fade=pl_anim_f(ctx,"_nf", visible?1.0f:0.0f, 0.15f);

    /* 定时消失：visible 转为 true 时记录 phase 起点；duration 后触发 on_close */
    if(visible && dur>0){
        float phase=pl_phase(ctx,"_np_global",10000.0f);  /* 极大周期 ≈ 单调时钟 */
        PlSignal ts=pl_state_f(ctx,"_nts",-1.0f);
        float t0=pl_f(ts);
        if(t0<0){ pl_set_f(ts,phase); t0=phase; }   /* 首次显示：记录起始时间 */
        if(phase-t0>=dur){
            /* 时间到：触发 close */
            PlNFire_ ev={.kind=kPlEv_TextInput,.text="timeout"};
            if(cb) cb(ctx,(PlEvent)(void*)&ev);
            pl_set_f(ts,-1.0f);   /* 重置，下次显示重新计时 */
        }
    } else if(!visible){
        /* 隐藏时重置计时器 */
        pl_set_f(pl_state_f(ctx,"_nts",-1.0f),-1.0f);
    }

    if(fade<0.01f) return pl_spacer(ctx);

    PlColor bg,tc; notify_colors(variant,&bg,&tc);

    /* 图标 */
    const char* icon;
    switch(variant){
        case kPlNotify_Success: icon="\xE2\x9C\x93"; break;   /* ✓ */
        case kPlNotify_Warning: icon="\xE2\x9A\xA0"; break;   /* ⚠ */
        case kPlNotify_Error:   icon="\xE2\x9C\x95"; break;   /* ✕ */
        default:                icon="\xE2\x84\xB9"; break;   /* ℹ */
    }

    PlNode icon_txt=pl_make(ctx,"pl.Text");
    pl_node_set_str  (icon_txt,kPlProp_Text,icon);
    pl_node_set_f    (icon_txt,kPlProp_FontSize,16.0f);
    pl_node_set_color(icon_txt,kPlProp_Color,tc);
    pl_node_set_bool (icon_txt,kPlPropText_InkCenter,true);   /* 按墨迹盒居中：补偿不同图标字形偏移 */

    PlNode msg_txt=pl_make(ctx,"pl.Text");
    pl_node_set_str  (msg_txt,kPlProp_Text,msg);
    pl_node_set_f    (msg_txt,kPlProp_FontSize,pl_theme_font(ctx,kPlType_Body));
    pl_node_set_color(msg_txt,kPlProp_Color,tc);
    pl_node_set_bool (msg_txt,kPlPropText_InkCenter,true);   /* 按墨迹盒居中：补偿 CJK 字形上下偏移 */

    /* 关闭 × 按钮：用 Box 包裹增大命中区（裸 Text 命中区过小几乎无法点中） */
    PlNode x_txt=pl_make(ctx,"pl.Text");
    pl_node_set_str  (x_txt,kPlProp_Text,"\xC3\x97");
    pl_node_set_f    (x_txt,kPlProp_FontSize,14.0f);
    pl_node_set_color(x_txt,kPlProp_Color,tc);
    pl_node_set_bool (x_txt,kPlPropText_InkCenter,true);
    PlNode x_btn=pl_make(ctx,"pl.Box");
    pl_node_set_edge (x_btn,kPlProp_Padding,pl_all(pl_px(4)));
    pl_node_set_f    (x_btn,kPlProp_Radius,4.0f);
    pl_add_child(x_btn,x_txt);
    pl_on(x_btn,kPlEv_Tap,notify_close);

    PlNode row=pl_make(ctx,"pl.Row");
    pl_node_set_enum(row,kPlProp_Align,kPlAlign_Center);
    pl_node_set_enum(row,kPlProp_Justify,kPlAlign_Start);   /* 左对齐，× 紧跟消息后 */
    pl_node_set_f   (row,kPlProp_Gap,8.0f);
    pl_add_child(row,icon_txt); pl_add_child(row,msg_txt); pl_add_child(row,x_btn);

    PlNode card=pl_make(ctx,"pl.Box");
    pl_node_set_color(card,kPlProp_Background,bg);
    pl_node_set_f    (card,kPlProp_Radius,8.0f);
    pl_node_set_edge (card,kPlProp_Padding,pl_sym(pl_px(14),pl_px(10)));
    pl_node_set_f    (card,kPlProp_Opacity,fade);
    pl_node_set_len  (card,kPlProp_MinWidth,pl_px(280));
    pl_add_child(card,row);

    /* 内联渲染：通知条就地占位（放进 Column 即自然纵向堆叠），不再用 Portal 浮层。
     * 之前的 Portal 因为拿不到视口宽，固定落在左上角并互相重叠盖住顶栏——那是 bug。
     * 需要 toast（角落浮层 + 自动消失）时，外层用 pl.Portal 包一层即可；自动消失逻辑此处仍保留。 */
    return card;
}

static const PlWidgetVTable kNotifyVT={.struct_size=sizeof(PlWidgetVTable),.type_name="pl.Notification",.build=notify_build};
void pl_notification_register(void){ pl_register_widget(&kNotifyVT); }
