// // StyleManager.cpp
// #include "StyleManager.h"

// QString StyleManager::getGlobalStyleSheet()
// {
//     return R"(
//         /* === 全局基础样式 === */
//         * {
//             color: white;
//         }

//         /* === 主窗口样式 === */
//         QMainWindow {
//             background-color: #1a1a2e;
//         }

//         /* === 对话框样式 === */
//         QDialog {
//             background-color: #1a1a2e;
//             color: white;
//         }

//         // /* === 组合框样式 === */
//         // QGroupBox {
//         //     font-size: 13px;
//         //     font-weight: bold;
//         //     color: #e94560;
//         //     border: 2px solid #3a3a5e;
//         //     border-radius: 5px;
//         //     margin-top: 10px;
//         //     padding-top: 10px;
//         // }
//         // QGroupBox::title {
//         //     subcontrol-origin: margin;
//         //     left: 20px;
//         //     padding: 0 10px 0 10px;
//         // }

//         // /* === 输入控件统一样式 === */
//         // QLineEdit, QTextEdit, QPlainTextEdit {
//         //     background-color: #0f3460;
//         //     color: white;
//         //     border: 1px solid #3a3a5e;
//         //     padding: 5px;
//         //     border-radius: 3px;
//         //     selection-background-color: #533483;
//         // }

//             // QComboBox {
//             //     background-color: #0f3460;
//             //     color: white;
//             //     border: 1px solid #3a3a5e;
//             //     padding: 5px;
//             //     border-radius: 3px;
//             //     selection-background-color: #533483;
//             // }
//         // QComboBox::drop-down {
//         //     border: none;
//         //     width: 20px;
//         // }
//         // QComboBox::down-arrow {
//         //     image: none;
//         //     border-left: 4px solid transparent;
//         //     border-right: 4px solid transparent;
//         //     border-top: 6px solid white;
//         //     margin-right: 5px;
//         // }这会造成类的箭头变成白色方块

//         // QComboBox QAbstractItemView {
//         //     background-color: #0f3460;
//         //     color: white;
//         //     border: 1px solid #3a3a5e;
//         //     selection-background-color: #533483;
//         // }

//         /* === 数字输入框统一样式）=== */
//         // QSpinBox, QDoubleSpinBox {
//         //     background-color: #0f3460;
//         //     color: white;
//         //     border: 1px solid #3a3a5e;
//         //     padding: 5px;
//         //     padding-right: 20px; /* 为箭头按钮留出空间 */
//         //     border-radius: 3px;
//         //     selection-background-color: #533483;
//         //     min-height: 20px;
//         // }
//         // QSpinBox::up-button, QDoubleSpinBox::up-button {
//         //     subcontrol-origin: border;
//         //     subcontrol-position: top right;
//         //     width: 18px;
//         //     height: 50%;
//         //     border-left: 1px solid #3a3a5e;
//         //     border-top-right-radius: 3px;
//         //     background-color: #533483;
//         // }
//         // QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover {
//         //     background-color: #e94560;
//         // }
//         // QSpinBox::up-button:pressed, QDoubleSpinBox::up-button:pressed {
//         //     background-color: #c23652;
//         // }
//         // QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
//         //     image: none;
//         //     border-style: solid;
//         //     border-width: 4px;
//         //     border-color: transparent transparent white transparent;
//         // }
//         // QSpinBox::down-button, QDoubleSpinBox::down-button {
//         //     subcontrol-origin: border;
//         //     subcontrol-position: bottom right;
//         //     width: 18px;
//         //     height: 50%;
//         //     border-left: 1px solid #3a3a5e;
//         //     border-bottom-right-radius: 3px;
//         //     background-color: #533483;
//         // }
//         // QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
//         //     background-color: #e94560;
//         // }
//         // QSpinBox::down-button:pressed, QDoubleSpinBox::down-button:pressed {
//         //     background-color: #c23652;
//         // }
//         // QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
//         //     image: none;
//         //     border-style: solid;
//         //     border-width: 4px;
//         //     border-color: white transparent transparent transparent;
//         // }

//         /* === 按钮统一样式 === */
//         QPushButton {
//             background-color: #533483;
//             color: white;
//             border: none;
//             padding: 8px 15px;
//             border-radius: 4px;
//             font-weight: bold;
//             min-height: 20px;
//         }
//         QPushButton:hover {
//             background-color: #e94560;
//         }
//         QPushButton:pressed {
//             background-color: #c23652;
//         }
//         QPushButton:disabled {
//             background-color: #3a3a5e;
//             color: #888;
//         }

//         /* === 按钮样式类 === */
//         QPushButton[class="success"] {
//             background-color: #22c55e;
//         }
//         QPushButton[class="success"]:hover {
//             background-color: #16a34a;
//         }
//         QPushButton[class="danger"] {
//             background-color: #ef4444;
//         }
//         QPushButton[class="danger"]:hover {
//             background-color: #dc2626;
//         }
//         QPushButton[class="warning"] {
//             background-color: #f59e0b;
//         }
//         QPushButton[class="warning"]:hover {
//             background-color: #d97706;
//         }
//         QPushButton[class="info"] {
//             background-color: #3b82f6;
//         }
//         QPushButton[class="info"]:hover {
//             background-color: #2563eb;
//         }
//         QPushButton[class="emergency"] {
//             background-color: #e94560;
//             font-size: 14px;
//             padding: 10px 20px;
//             font-weight: bold;
//         }
//         QPushButton[class="emergency"]:hover {
//             background-color: #ff1744;
//         }

//         /* === 其他控件样式 === */
//         QRadioButton, QCheckBox {
//             color: white;
//             font-size: 12px;
//             spacing: 5px;
//         }

//         QLabel {
//             color: white;
//         }

//         QTabWidget::pane {
//             border: 1px solid #3a3a5e;
//             background-color: #16213e;
//         }
//         QTabBar::tab {
//             background-color: #0f3460;
//             color: white;
//             padding: 10px 20px;
//             margin-right: 2px;
//             font-size: 14px;
//             font-weight: bold;
//             border-top-left-radius: 5px;
//             border-top-right-radius: 5px;
//         }
//         QTabBar::tab:selected {
//             background-color: #e94560;
//         }
//         QTabBar::tab:hover {
//             background-color: #533483;
//         }

//         QToolBar {
//             background-color: #0f3460;
//             border: none;
//             padding: 5px;
//             spacing: 5px;
//         }
//         QToolBar QPushButton {
//             margin: 0 2px;
//             padding: 8px 15px;
//             font-size: 13px;
//         }

//         QStatusBar {
//             background-color: #0f3460;
//             color: white;
//             border-top: 1px solid #3a3a5e;
//         }
//         QStatusBar QLabel {
//             color: white;
//             padding: 0 10px;
//         }
//         QStatusBar QPushButton {
//             padding: 5px 10px;
//             font-size: 11px;
//         }

//         QScrollBar:vertical {
//             background-color: #1a1a2e;
//             width: 12px;
//             border-radius: 6px;
//         }
//         QScrollBar::handle:vertical {
//             background-color: #533483;
//             border-radius: 6px;
//             min-height: 20px;
//         }
//         QScrollBar::handle:vertical:hover {
//             background-color: #e94560;
//         }

//         QToolTip {
//             background-color: #0f3460;
//             color: white;
//             border: 1px solid #3a3a5e;
//             padding: 5px;
//             border-radius: 3px;
//             font-size: 12px;
//         }
//     )";
// }

// // // 获取特定组件样式的方法可以根据需要实现
// // QString StyleManager::getSpinBoxStyle()
// // {
// //     return R"(
// //         QSpinBox, QDoubleSpinBox {
// //             background-color: #0f3460;
// //             color: white;
// //             border: 1px solid #3a3a5e;
// //             padding: 5px;
// //             border-radius: 3px;
// //         }
// //     )";
// // }
