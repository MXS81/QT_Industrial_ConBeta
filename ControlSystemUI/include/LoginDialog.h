#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    QString getUsername() const; // 公共接口，用于获取登录成功的用户名

private slots:
    void onLoginClicked();

private:
    void setupUI();

    QLabel *m_userLabel;
    QLabel *m_passwordLabel;
    QLineEdit *m_userEdit;
    QLineEdit *m_passwordEdit;
    QPushButton *m_loginButton;
    QPushButton *m_cancelButton;

    QString m_username; // 用于存储登录成功的用户名
};

#endif // LOGINDIALOG_H
