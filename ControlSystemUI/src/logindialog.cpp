#include "LoginDialog.h"
#include <QGridLayout>
#include <QMessageBox>
#include <QMap>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent)
{
    setupUI();
    setWindowTitle("SKZR轨道控制系统 - 用户登录");
    setModal(true); // 设置为模态对话框
}

void LoginDialog::setupUI()
{
    m_userLabel = new QLabel("用户名:");
    m_passwordLabel = new QLabel("密  码:");
    m_userEdit = new QLineEdit(this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password); // 设置密码输入模式

    m_loginButton = new QPushButton("登录");
    m_cancelButton = new QPushButton("取消");

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(m_userLabel, 0, 0);
    layout->addWidget(m_userEdit, 0, 1);
    layout->addWidget(m_passwordLabel, 1, 0);
    layout->addWidget(m_passwordEdit, 1, 1);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_loginButton);
    buttonLayout->addWidget(m_cancelButton);
    layout->addLayout(buttonLayout, 2, 0, 1, 2);

    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject); // 取消按钮关闭对话框
}

void LoginDialog::onLoginClicked()
{
    // !!! 警告：这是一个示例用的硬编码用户列表。
    // !!! 在实际生产环境中，绝不能这样存储密码！
    // !!! 应该使用数据库和加密哈希算法。
    QMap<QString, QString> users;
    users["admin"] = "1";
    users["operator"] = "2";

    QString username = m_userEdit->text();
    QString password = m_passwordEdit->text();

    if (users.contains(username) && users.value(username) == password) {
        m_username = username; // 存储用户名
        accept(); // 发送 accept 信号，表示登录成功，并关闭对话框
    } else {
        QMessageBox::warning(this, "登录失败", "用户名或密码错误！");
        m_passwordEdit->clear();
        m_userEdit->selectAll();
        m_userEdit->setFocus();
    }
}

QString LoginDialog::getUsername() const
{
    return m_username;
}
