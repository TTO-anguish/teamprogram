#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QTcpServer> //服务器类
#include <QCoreApplication>
#include <QAbstractSocket>
#include <QDebug>
#include <QMessageBox> //消息对话框类
#include <QVBoxLayout>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QTimer>
#include <QThread>
#include <iostream>
#include <QDateTime>
#include <QMenu>
#include <QComboBox>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include "QtMqtt/qmqttclient.h"
#include <QtMqtt/qmqttmessage.h>
#include <QJsonObject>
#include <QJsonDocument>
#include "sensor.h"

using namespace QtCharts;

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE



class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Ui::Widget *ui;
    QMqttClient *client;
    QTimer *timer;
    QString formattedText;
    QTimer *updateTimer;        // 定时器用于每秒更新一次UI
        corrToQt_t latestData;      // 用于存储最新的接收到的数据
        bool hasNewData;            // 标志是否有新的数据需要处理
    // 辅助函数：序列化和反序列化total_t数据
    QByteArray serializeTotal(const corrToQt_t &data);
    corrToQt_t deserializeTotal(const QByteArray &jsonData);
corrToQt_t generateSampleData();

private slots:
    void onConnectButtonClicked();
    void onMessageReceived(const QByteArray &message, const QMqttTopicName &topic);
    void onMqttStateChanged(QMqttClient::ClientState state);
    void updateTime();
    void updateUiWithReceivedData(const corrToQt_t &data);
void updateUiPeriodically();
    void on_pushButton_clicked();
    void on_discon_btn_clicked();
    void on_send_cmd_btn_2_clicked();
    void on_send_cmd_btn_clicked();
};
#endif // WIDGET_H
