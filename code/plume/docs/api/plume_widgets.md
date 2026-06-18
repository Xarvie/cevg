# Plume 控件 API 参考

> 自动生成自 `schema/plume.schema.json`（`tools/gen_schema.py`）——请勿手改；改 schema 后重跑生成器。
> 共 42 个控件。每条给出 L2 宏用法与 L1 desc 字段（名 / 类型 / 说明）；未设字段默认 0 = 用控件默认值，颜色用完整 `0xRRGGBBAA` 或 `pl_hex(0xRRGGBB)`。

**控件**：`pl.Box` · `pl.Row` · `pl.Column` · `pl.Stack` · `pl.Portal` · `pl.Text` · `pl.Image` · `pl.Button` · `pl.Spacer` · `pl.Divider` · `pl.Scroll` · `pl.Grid` · `pl.Input` · `pl.TextArea` · `pl.Checkbox` · `pl.Switch` · `pl.Slider` · `pl.Select` · `pl.Progress` · `pl.Modal` · `pl.Tooltip` · `pl.Radio` · `pl.Badge` · `pl.Chip` · `pl.Accordion` · `pl.Tabs` · `pl.Avatar` · `pl.List` · `pl.Notification` · `pl.RichText` · `pl.Menu` · `pl.Pagination` · `pl.ColorPicker` · `pl.DatePicker` · `pl.Stepper` · `pl.Rating` · `pl.Timeline` · `pl.Table` · `pl.SplitPane` · `pl.Carousel` · `pl.SearchBar` · `pl.Breadcrumb`

---

### `pl.Box`　<sub>render</sub>

盒：单子容器（尺寸/内边距/背景/圆角/边框/裁剪/min-max/grow-shrink）

用法：`PL_BOX(ctx, .字段=值, …)`　·　函数：`pl_box(ctx, (PlBoxDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `width` | `float` | 0=未设(自动按内容) |
| `height` | `float` | 0=未设(自动按内容) |
| `min_width` | `float` | 尺寸边界；0=未设 |
| `min_height` | `float` | 尺寸边界；0=未设 |
| `max_width` | `float` | 尺寸边界；0=未设 |
| `max_height` | `float` | 尺寸边界；0=未设 |
| `bg` | `PlColor` | 0xRRGGBBAA；0=未设(透明) |
| `radius` | `float` | 0=未设(直角) |
| `clip` | `bool` | 裁剪溢出子(含圆角)；默认 false |
| `padding` | `float` | 统一内边距；0=未设(无) |
| `border_color` | `PlColor` | 边框色（仅 border_width 设时生效） |
| `border_width` | `float` | 0=未设(无边框) |
| `opacity` | `float` | 0=未设(=1) |
| `grow` | `float` | flex 主轴 grow；0=未设 |
| `shrink` | `float` | flex 主轴 shrink；0=未设(不收缩) |
| `on_tap` | `PlHandler` | 点击处理器；NULL=无 |
| `child` | `PlNode` | 可选单子 |

---

### `pl.Row`　<sub>render</sub>

水平 flex

用法：`PL_ROW(ctx, .字段=值, …)`　·　函数：`pl_row(ctx, (PlRowDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `gap` | `float` | 项目间距；0=未设(默认 8) |
| `justify` | `int` | 主轴对齐 kPlJustify_* |
| `align` | `int` | 交叉轴对齐 kPlAlign_* |
| `children` | `PlNode[]` | 子项（顺序填，遇 NULL 止） |

---

### `pl.Column`　<sub>render</sub>

竖直 flex

用法：`PL_COLUMN(ctx, .字段=值, …)`　·　函数：`pl_column(ctx, (PlColumnDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `gap` | `float` | 项目间距；0=未设(默认 12) |
| `justify` | `int` | 主轴对齐 kPlJustify_* |
| `align` | `int` | 交叉轴对齐 kPlAlign_* |
| `children` | `PlNode[]` | 子项（顺序填，遇 NULL 止） |

---

### `pl.Stack`　<sub>render</sub>

重叠分层（z 序=子序）

用法：`PL_STACK(ctx, .字段=值, …)`　·　函数：`pl_stack(ctx, (PlStackDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `width` | `float` | 0=未设(子最大) |
| `height` | `float` | 0=未设(子最大) |
| `justify` | `int` | 水平对齐 kPlJustify_Start/Center/End |
| `align` | `int` | 竖直对齐 kPlAlign_Start/Center/End |
| `children` | `PlNode[]` | 子项（先者在底，遇 NULL 止） |

---

### `pl.Portal`　<sub>render</sub>

顶层浮层：子树传送到根坐标最上层，逃逸祖先 transform/clip/opacity

用法：`PL_PORTAL(ctx, .字段=值, …)`　·　函数：`pl_portal(ctx, (PlPortalDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `x` | `float` | 放置 x（根坐标；anchor 时为相对容器偏移）；0=左上/原位 |
| `y` | `float` | 放置 y（根坐标；anchor 时为相对容器偏移）；0=左上/原位 |
| `z` | `int` | 层叠序（大者在上）；0=默认 |
| `anchor` | `bool` | 锚定：x/y 改作相对偏移，基准=portal 所在容器布局后位置（浮层随触发处移动） |
| `flip` | `bool` | 边界翻转（仅锚定）：默认下方会溢出视口底边且上方放得下时，自动翻到触发处上方 |
| `child` | `PlNode` | 浮层内容 |

---

### `pl.Text`　<sub>render</sub>

文本（cevg 排版；省略号/换行）

用法：`PL_TEXT(ctx, .字段=值, …)`　·　函数：`pl_text(ctx, (PlTextDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `text` | `const char*` | 文本（拷贝） |
| `size` | `float` | 字号；0=未设(16) |
| `color` | `PlColor` | 0xRRGGBBAA 或 pl_hex；0=未设(黑) |
| `grow` | `float` | flex grow |
| `ellipsis` | `bool` | 放不下尾部省略号 |
| `wrap` | `bool` | 超宽按词换行 |
| `dir` | `int` | 文本方向 kPlDir_Auto/LTR/RTL |

---

### `pl.Image`　<sub>render</sub>

位图（缩放 + 圆角裁剪）

用法：`PL_IMAGE(ctx, .字段=值, …)`　·　函数：`pl_image(ctx, (PlImageDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `src` | `CevgImage*` | 图像（宿主持有生命周期） |
| `width` | `float` | 0=未设(图像自然尺寸) |
| `height` | `float` | 0=未设(图像自然尺寸) |
| `radius` | `float` | 圆角裁剪；0=未设(方角) |
| `grow` | `float` | flex grow |

---

### `pl.Button`　<sub>composite</sub>

按钮（按下态 + on_press 动作）

用法：`PL_BUTTON(ctx, .字段=值, …)`　·　函数：`pl_button(ctx, (PlButtonDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `label` | `const char*` | 按钮文字 |
| `bg` | `PlColor` | 底色；0=未设(默认蓝) |
| `color` | `PlColor` | 文字色；0=未设(默认白) |
| `radius` | `float` | 圆角；0=未设(10) |
| `width` | `float` | 0=未设(按内容) |
| `height` | `float` | 0=未设(按内容) |
| `font_size` | `float` | 0=未设(20) |
| `on_press` | `PlHandler` | 点击动作 |

---

### `pl.Spacer`　<sub>render</sub>

弹性空白（grow=1）

用法：`PL_SPACER(ctx)`（无字段）　·　函数：`pl_spacer(ctx)`

---

### `pl.Divider`　<sub>render</sub>

分隔线（横/竖）

用法：`PL_DIVIDER(ctx, .字段=值, …)`　·　函数：`pl_divider(ctx, (PlDividerDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `color` | `PlColor` | 线色；0=未设(浅灰) |
| `thickness` | `float` | 厚度；0=未设(1) |
| `vertical` | `bool` | 竖向（填高×厚） |

---

### `pl.Scroll`　<sub>render</sub>

滚动视口（裁剪 + 偏移 + 滚动条）

用法：`PL_SCROLL(ctx, .字段=值, …)`　·　函数：`pl_scroll(ctx, (PlScrollDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `width` | `float` | 视口宽；0=未设(按约束) |
| `height` | `float` | 视口高；0=未设(按约束) |
| `scroll` | `float` | 竖直滚动偏移（内容上移量） |
| `scroll_x` | `float` | 水平滚动偏移（内容左移量）；0=未设 |
| `scrollbar` | `bool` | 显示滚动条指示器（双轴：内容溢出哪轴显哪轴） |
| `child` | `PlNode` | 内容（可比视口高） |

---

### `pl.Grid`　<sub>render</sub>

N 列网格（等宽/自适应列宽 + 行高 minmax）

用法：`PL_GRID(ctx, .字段=值, …)`　·　函数：`pl_grid(ctx, (PlGridDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `cols` | `int` | 列数；0=未设(默认 2) |
| `gap` | `float` | 项目间距；0=未设 |
| `autofit` | `int` | 列宽按内容自适应；0=未设(等宽) |
| `min_w` | `float` | 自适应时每列最小宽 |
| `max_w` | `float` | 自适应时每列最大宽(0=不封顶) |
| `min_h` | `float` | 每行最小高(0=不保底) |
| `max_h` | `float` | 每行最大高(0=不封顶) |
| `children` | `PlNode[]` | 子项（顺序填，遇 NULL 止） |

---

### `pl.Input`　<sub>composite</sub>

单行文本输入框（含光标、聚焦环、placeholder、密码模式）

用法：`PL_INPUT(ctx, .字段=值, …)`　·　函数：`pl_input(ctx, (PlInputDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `value` | `str` | 当前值（受控）；NULL=无 |
| `placeholder` | `str` | 空值时的占位文字 |
| `on_change` | `PlHandler` | 值变化回调；pl_event_text=新值 |
| `on_submit` | `PlHandler` | Enter 提交回调 |
| `disabled` | `bool` | 禁用态 |
| `password` | `bool` | 密码模式（• 遮盖） |
| `width` | `float` | 0=未设(自动) |
| `height` | `float` | 0=未设(自动) |

---

### `pl.TextArea`　<sub>composite</sub>

多行文本编辑框（光标/换行/上下行导航/BiDi 双向文本）

用法：`PL_TEXTAREA(ctx, .字段=值, …)`　·　函数：`pl_textarea(ctx, (PlTextAreaDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `value` | `str` | 当前值（受控，可含 \n）；NULL=无 |
| `placeholder` | `str` | 空值时的占位文字 |
| `on_change` | `PlHandler` | 值变化回调；pl_event_text=新值 |
| `rows` | `int` | 可见行数（决定默认高度）；0=未设(4) |
| `dir` | `int` | 文本方向 kPlDir_Auto/LTR/RTL |
| `disabled` | `bool` | 禁用态 |
| `width` | `float` | 0=未设(自动) |
| `height` | `float` | 0=未设(按 rows) |

---

### `pl.Checkbox`　<sub>composite</sub>

复选框（勾选/取消，含对勾动画）

用法：`PL_CHECKBOX(ctx, .字段=值, …)`　·　函数：`pl_checkbox(ctx, (PlCheckboxDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `checked` | `bool` | 勾选状态 |
| `on_change` | `PlHandler` | 切换回调；pl_event_text=\"1\"/\"0\" |
| `disabled` | `bool` | 禁用态 |

---

### `pl.Switch`　<sub>composite</sub>

开关/切换（knob 动画 + 轨道色过渡）

用法：`PL_SWITCH(ctx, .字段=值, …)`　·　函数：`pl_switch(ctx, (PlSwitchDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `on` | `bool` | 开关状态 |
| `on_change` | `PlHandler` | 切换回调；pl_event_text=\"1\"/\"0\" |
| `disabled` | `bool` | 禁用态 |

---

### `pl.Slider`　<sub>composite</sub>

水平滑块（0–1 范围，可拖拽）

用法：`PL_SLIDER(ctx, .字段=值, …)`　·　函数：`pl_slider(ctx, (PlSliderDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `value` | `float` | 当前值 0.0–1.0 |
| `on_change` | `PlHandler` | 拖动回调；pl_event_text=\"0.0000\"–\"1.0000\" |
| `disabled` | `bool` | 禁用态 |

---

### `pl.Select`　<sub>composite</sub>

下拉选择（含分组/禁用/键盘导航，portal 浮层）

用法：`PL_SELECT(ctx, .字段=值, …)`　·　函数：`pl_select(ctx, (PlSelectDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `value` | `int` | 选中项索引（-1=未选） |
| `placeholder` | `str` | 未选中时的提示文字 |
| `on_change` | `PlHandler` | 选中变化回调 |
| `options` | `ptr` | const char** |
| `option_count` | `int` | 选项数 |
| `disabled` | `bool` | 禁用 |

---

### `pl.Progress`　<sub>composite</sub>

进度条（确定/不确定两种模式）

用法：`PL_PROGRESS(ctx, .字段=值, …)`　·　函数：`pl_progress(ctx, (PlProgressDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `value` | `float` | 0.0–1.0；< 0 = spinner |
| `thickness` | `float` | 0=默认 6px |
| `width` | `float` | 0=自动 |

---

### `pl.Modal`　<sub>composite</sub>

模态遮罩层（portal 顶层）

用法：`PL_MODAL(ctx, .字段=值, …)`　·　函数：`pl_modal(ctx, (PlModalDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `open` | `bool` | 是否显示 |
| `on_close` | `PlHandler` | 关闭回调 |
| `child` | `PlNode` | 内容 |

---

### `pl.Tooltip`　<sub>composite</sub>

提示气泡（hover 延迟出现）

用法：`PL_TOOLTIP(ctx, .字段=值, …)`　·　函数：`pl_tooltip(ctx, (PlTooltipDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `content` | `str` | 提示文字 |
| `delay` | `float` | 0=默认 0.3s |
| `child` | `PlNode` | 触发元素 |

---

### `pl.Radio`　<sub>composite</sub>

单选按钮

用法：`PL_RADIO(ctx, .字段=值, …)`　·　函数：`pl_radio(ctx, (PlRadioDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `checked` | `bool` | 选中 |
| `on_change` | `PlHandler` | 点击回调 |
| `disabled` | `bool` | 禁用 |

---

### `pl.Badge`　<sub>composite</sub>

数字徽章（叠加在子元素右上角）

用法：`PL_BADGE(ctx, .字段=值, …)`　·　函数：`pl_badge(ctx, (PlBadgeDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `count` | `int` | 数字；0=隐藏 |
| `content` | `str` | 文字（优先） |
| `color` | `PlColor` | 0=红 |
| `child` | `PlNode` | 被标注的内容 |

---

### `pl.Chip`　<sub>composite</sub>

标签/Chip（可选中、可删除）

用法：`PL_CHIP(ctx, .字段=值, …)`　·　函数：`pl_chip(ctx, (PlChipDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `label` | `str` | 文字 |
| `selected` | `bool` | 选中高亮 |
| `on_tap` | `PlHandler` | 点击 |
| `on_remove` | `PlHandler` | 删除（NULL=不显示×） |
| `disabled` | `bool` | 禁用 |

---

### `pl.Accordion`　<sub>composite</sub>

可展开折叠面板

用法：`PL_ACCORDION(ctx, .字段=值, …)`　·　函数：`pl_accordion(ctx, (PlAccordionDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `title` | `str` | 标题 |
| `open` | `bool` | 展开 |
| `on_toggle` | `PlHandler` | 点击标题 |
| `child` | `PlNode` | 折叠内容 |

---

### `pl.Tabs`　<sub>composite</sub>

Tab 切换栏（水平，激活项有下划线/高亮）

用法：`PL_TABS(ctx, .字段=值, …)`　·　函数：`pl_tabs(ctx, (PlTabsDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `active_index` | `int` | 激活索引 |
| `on_change` | `PlHandler` | 切换回调 |
| `labels` | `ptr` | const char** |
| `count` | `int` | 标签数 |

---

### `pl.Avatar`　<sub>composite</sub>

圆形头像（图片或首字母缩写）

用法：`PL_AVATAR(ctx, .字段=值, …)`　·　函数：`pl_avatar(ctx, (PlAvatarDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `initials` | `str` | 首字母 |
| `color` | `PlColor` | 0=自动 |
| `size` | `float` | 直径 px |
| `src` | `ptr` | CevgImage* |

---

### `pl.List`　<sub>composite</sub>

虚拟化等高列表（只渲染可视区域项）

用法：`PL_LIST(ctx, .字段=值, …)`　·　函数：`pl_list(ctx, (PlListDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `count` | `int` | 总项数 |
| `item_height` | `float` | 等高 px |
| `builder` | `ptr` | PlListBuilder |
| `scroll_y` | `float` | 滚动偏移 |
| `on_scroll` | `ptr` | 滚动回调 |
| `height` | `float` | 视口高 |
| `width` | `float` | 视口宽 |

---

### `pl.Notification`　<sub>composite</sub>

Toast 通知条（Portal 顶层，自动消失）

用法：`PL_NOTIFICATION(ctx, .字段=值, …)`　·　函数：`pl_notification(ctx, (PlNotificationDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `message` | `str` | 消息文字 |
| `duration` | `float` | 秒；0=不自动消 |
| `variant` | `int` | 0=Info 1=Success 2=Warn 3=Error |
| `on_close` | `PlHandler` | 消失回调 |
| `visible` | `bool` | 是否显示 |

---

### `pl.RichText`　<sub>composite</sub>

多段 inline 样式文本（bold/color/size 混排）

用法：`PL_RICHTEXT(ctx, .字段=值, …)`　·　函数：`pl_richtext(ctx, (PlRichTextDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `spans` | `ptr` | PlSpan* |
| `span_count` | `int` | 数量 |
| `wrap` | `bool` | 换行 |

---

### `pl.Menu`　<sub>composite</sub>

右键/弹出菜单（Portal 定位，MenuItem 列表）

用法：`PL_MENU(ctx, .字段=值, …)`　·　函数：`pl_menu(ctx, (PlMenuDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `open` | `bool` | 展开 |
| `items` | `ptr` | PlMenuItem* |
| `item_count` | `int` | 数量 |
| `on_select` | `PlHandler` | 选中回调 |
| `x` | `float` | 弹出 x |
| `y` | `float` | 弹出 y |

---

### `pl.Pagination`　<sub>composite</sub>

分页控件（< 1 2 3 … N >）

用法：`PL_PAGINATION(ctx, .字段=值, …)`　·　函数：`pl_pagination(ctx, (PlPaginationDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `page` | `int` | 当前页 |
| `page_count` | `int` | 总页数 |
| `on_change` | `PlHandler` | 切换回调 |

---

### `pl.ColorPicker`　<sub>composite</sub>

颜色选择器（HSL 滑块）

用法：`PL_COLORPICKER(ctx, .字段=值, …)`　·　函数：`pl_colorpicker(ctx, (PlColorPickerDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `value` | `PlColor` | 颜色 |
| `on_change` | `PlHandler` | 回调 |

---

### `pl.DatePicker`　<sub>composite</sub>

日期选择器（月历网格）

用法：`PL_DATEPICKER(ctx, .字段=值, …)`　·　函数：`pl_datepicker(ctx, (PlDatePickerDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `year` | `int` | 年 |
| `month` | `int` | 月 1-12 |
| `day` | `int` | 日 1-31 |
| `on_change` | `PlHandler` | 回调 |

---

### `pl.Stepper`　<sub>composite</sub>

数字步进器（+/- 按钮 + 值显示）

用法：`PL_STEPPER(ctx, .字段=值, …)`　·　函数：`pl_stepper(ctx, (PlStepperDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `value` | `int` | 当前值 |
| `min` | `int` | 最小值 |
| `max` | `int` | 最大值 |
| `step` | `int` | 步进量 |
| `on_change` | `PlHandler` | 回调 |
| `disabled` | `bool` | 禁用 |

---

### `pl.Rating`　<sub>composite</sub>

评分星级（可交互/只读）

用法：`PL_RATING(ctx, .字段=值, …)`　·　函数：`pl_rating(ctx, (PlRatingDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `value` | `int` | 评分 0-5 |
| `max` | `int` | 最大星数 |
| `on_change` | `PlHandler` | 回调 |
| `read_only` | `bool` | 只读 |

---

### `pl.Timeline`　<sub>composite</sub>

时间线（垂直步骤列表）

用法：`PL_TIMELINE(ctx, .字段=值, …)`　·　函数：`pl_timeline(ctx, (PlTimelineDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `items` | `ptr` | PlTimelineItem* |
| `item_count` | `int` | 数量 |

---

### `pl.Table`　<sub>composite</sub>

数据表格（表头+行，可选中行）

用法：`PL_TABLE(ctx, .字段=值, …)`　·　函数：`pl_table(ctx, (PlTableDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `columns` | `ptr` | PlTableColumn* |
| `column_count` | `int` | 列数 |
| `rows` | `ptr` | const char** |
| `row_count` | `int` | 行数 |
| `on_row_select` | `ptr` | 回调 |
| `selected_row` | `int` | 选中行 |

---

### `pl.SplitPane`　<sub>composite</sub>

可拖拽分割面板（左右或上下）

用法：`PL_SPLITPANE(ctx, .字段=值, …)`　·　函数：`pl_splitpane(ctx, (PlSplitPaneDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `split` | `float` | 比例 0-1 |
| `vertical` | `bool` | 左右分割 |
| `on_change` | `PlHandler` | 回调 |
| `first` | `ptr` | 第一面板 |
| `second` | `ptr` | 第二面板 |
| `width` | `float` | 总宽 |
| `height` | `float` | 总高 |

---

### `pl.Carousel`　<sub>composite</sub>

横向轮播组件（滑动切换）

用法：`PL_CAROUSEL(ctx, .字段=值, …)`　·　函数：`pl_carousel(ctx, (PlCarouselDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `index` | `int` | 当前项 |
| `count` | `int` | 总数 |
| `builder` | `ptr` | PlListBuilder |
| `on_change` | `PlHandler` | 切换回调 |
| `width` | `float` | 项宽 px |
| `height` | `float` | 高度 px |

---

### `pl.SearchBar`　<sub>composite</sub>

搜索栏（带图标和清除按钮的 Input 封装）

用法：`PL_SEARCHBAR(ctx, .字段=值, …)`　·　函数：`pl_searchbar(ctx, (PlSearchBarDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `value` | `str` | 当前文字 |
| `placeholder` | `str` | 提示文字 |
| `on_change` | `PlHandler` | 变更回调 |
| `on_submit` | `PlHandler` | 提交回调 |
| `on_clear` | `PlHandler` | 清除回调 |

---

### `pl.Breadcrumb`　<sub>composite</sub>

面包屑导航（路径 > 链接）

用法：`PL_BREADCRUMB(ctx, .字段=值, …)`　·　函数：`pl_breadcrumb(ctx, (PlBreadcrumbDesc){ … })`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `items` | `ptr` | const char** |
| `item_count` | `int` | 数量 |
| `on_click` | `ptr` | 点击回调 |

---

## 速查（紧凑签名，便于整段粘贴给模型）

```
pl.Box(width:float, height:float, min_width:float, min_height:float, max_width:float, max_height:float, bg:PlColor, radius:float, clip:bool, padding:float, border_color:PlColor, border_width:float, opacity:float, grow:float, shrink:float, on_tap:PlHandler, child:PlNode)
pl.Row(gap:float, justify:int, align:int, children:PlNode[])
pl.Column(gap:float, justify:int, align:int, children:PlNode[])
pl.Stack(width:float, height:float, justify:int, align:int, children:PlNode[])
pl.Portal(x:float, y:float, z:int, anchor:bool, flip:bool, child:PlNode)
pl.Text(text:const char*, size:float, color:PlColor, grow:float, ellipsis:bool, wrap:bool, dir:int)
pl.Image(src:CevgImage*, width:float, height:float, radius:float, grow:float)
pl.Button(label:const char*, bg:PlColor, color:PlColor, radius:float, width:float, height:float, font_size:float, on_press:PlHandler)
pl.Spacer()
pl.Divider(color:PlColor, thickness:float, vertical:bool)
pl.Scroll(width:float, height:float, scroll:float, scroll_x:float, scrollbar:bool, child:PlNode)
pl.Grid(cols:int, gap:float, autofit:int, min_w:float, max_w:float, min_h:float, max_h:float, children:PlNode[])
pl.Input(value:str, placeholder:str, on_change:PlHandler, on_submit:PlHandler, disabled:bool, password:bool, width:float, height:float)
pl.TextArea(value:str, placeholder:str, on_change:PlHandler, rows:int, dir:int, disabled:bool, width:float, height:float)
pl.Checkbox(checked:bool, on_change:PlHandler, disabled:bool)
pl.Switch(on:bool, on_change:PlHandler, disabled:bool)
pl.Slider(value:float, on_change:PlHandler, disabled:bool)
pl.Select(value:int, placeholder:str, on_change:PlHandler, options:ptr, option_count:int, disabled:bool)
pl.Progress(value:float, thickness:float, width:float)
pl.Modal(open:bool, on_close:PlHandler, child:PlNode)
pl.Tooltip(content:str, delay:float, child:PlNode)
pl.Radio(checked:bool, on_change:PlHandler, disabled:bool)
pl.Badge(count:int, content:str, color:PlColor, child:PlNode)
pl.Chip(label:str, selected:bool, on_tap:PlHandler, on_remove:PlHandler, disabled:bool)
pl.Accordion(title:str, open:bool, on_toggle:PlHandler, child:PlNode)
pl.Tabs(active_index:int, on_change:PlHandler, labels:ptr, count:int)
pl.Avatar(initials:str, color:PlColor, size:float, src:ptr)
pl.List(count:int, item_height:float, builder:ptr, scroll_y:float, on_scroll:ptr, height:float, width:float)
pl.Notification(message:str, duration:float, variant:int, on_close:PlHandler, visible:bool)
pl.RichText(spans:ptr, span_count:int, wrap:bool)
pl.Menu(open:bool, items:ptr, item_count:int, on_select:PlHandler, x:float, y:float)
pl.Pagination(page:int, page_count:int, on_change:PlHandler)
pl.ColorPicker(value:PlColor, on_change:PlHandler)
pl.DatePicker(year:int, month:int, day:int, on_change:PlHandler)
pl.Stepper(value:int, min:int, max:int, step:int, on_change:PlHandler, disabled:bool)
pl.Rating(value:int, max:int, on_change:PlHandler, read_only:bool)
pl.Timeline(items:ptr, item_count:int)
pl.Table(columns:ptr, column_count:int, rows:ptr, row_count:int, on_row_select:ptr, selected_row:int)
pl.SplitPane(split:float, vertical:bool, on_change:PlHandler, first:ptr, second:ptr, width:float, height:float)
pl.Carousel(index:int, count:int, builder:ptr, on_change:PlHandler, width:float, height:float)
pl.SearchBar(value:str, placeholder:str, on_change:PlHandler, on_submit:PlHandler, on_clear:PlHandler)
pl.Breadcrumb(items:ptr, item_count:int, on_click:ptr)
```
