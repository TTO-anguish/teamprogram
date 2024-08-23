#include "widget.h"
#include "sensor.h"
#include <QtCharts>
QT_CHARTS_USE_NAMESPACE
#include "ui_widget.h"
#include "QtMqtt/qmqttclient.h"
#include <QtMqtt/qmqttmessage.h>
#include <QMessageBox>

#define PUB "QtToUbuntu"
#define SUB "UbuntuToQt"


using namespace QtCharts;

// 使用QMqttClient替代QTcpSocket
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , timer(new QTimer(this))
    , client(new QMqttClient(this))
    , hasNewData(false) // 初始化标志为 false
{
    ui->setupUi(this);
    connect(timer, &QTimer::timeout, this, &Widget::updateTime);
    timer->start(1000);
    //界面题目
    ui->title->setText("管理系统");
    // 使用样式表设置文本样式
    ui->title->setStyleSheet(
                "QLabel {"
                "   font-family: 'Comic Sans MS';" // 设置字体类型
                "   font-size: 18pt;" // 设置字体大小
                "   font-weight: bold;" // 设置字体加粗
                "   color: #000000;" // 设置字体颜色
                "   background-color: #FFFFFF;" // 设置背景颜色
                "}"
                );
    // 初始化MQTT客户端，设置服务器地址和端口，并连接到服务器
    client->setHostname("test.mosquitto.org");
    client->setPort(1883); // 默认MQTT端口
    client->connectToHost();

    // 初始化更新定时器
        updateTimer = new QTimer(this);
        connect(updateTimer, &QTimer::timeout, this, &Widget::updateUiPeriodically);
        updateTimer->start(1000);

    // 连接信号与槽，用于处理按钮点击、消息接收和状态变化
    connect(client,&QMqttClient::connected, this, &Widget::onConnectButtonClicked);
    //断开按钮连接信号与曹
    connect(ui->discon_btn, &QPushButton::clicked, this, &Widget::on_discon_btn_clicked);
    connect(client, &QMqttClient::messageReceived, this, &Widget::onMessageReceived);
    connect(client, &QMqttClient::stateChanged, this, &Widget::onMqttStateChanged);

    connect(ui->pushButton, &QPushButton::clicked, this, &Widget::on_pushButton_clicked);
}

Widget::~Widget()
{
    delete ui;
}
// 处理连接事件的槽函数
void Widget::onConnectButtonClicked()
{
    // 订阅一个主题
    client->subscribe((QString)SUB);


}
//断开按钮事件槽函数
void Widget::on_discon_btn_clicked()
{
    client->disconnectFromHost(); // 断开与 MQTT 代理的连接
}
// 处理接收到的 MQTT 消息的槽函数
void Widget::onMessageReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    // 接收到MQTT消息时的处理逻辑
   qDebug() << "Received message on topic" << topic.name() << ":" << message.data();

    // 将接收到的JSON数据反序列化为corrToQt_t结构体
        latestData = deserializeTotal(message);
        hasNewData = true; // 标记为有新的数据需要处理
}
//检查是否有新的数据，并更新UI
void Widget::updateUiPeriodically()
{
    if (hasNewData) // 只有在有新数据时才更新UI
    {
        updateUiWithReceivedData(latestData);
        hasNewData = false; // 重置标志
    }
}
// 更新UI显示数据
void Widget::updateUiWithReceivedData(const corrToQt_t &data)
{
    ui->label_hum_2->setText(QString::number(data.enviro.hum));
    ui->label_tem_2->setText(QString::number(data.enviro.tem));
    ui->label_sun_2->setText(QString::number(data.enviro.sunshine));
    ui->label_nh3_2->setText(QString::number(data.enviro.NH3));

    if (data.warn.flag_NH3)
    {
        ui->textEdit_echo->setText("氨气浓度过高");
    }
}
// 处理 MQTT 客户端状态变化的槽函数
void Widget::onMqttStateChanged(QMqttClient::ClientState state)
{
    // 处理MQTT状态变化
    if (state == QMqttClient::Connected)
    {
        qDebug() << "MQTT connected";
    }
    else if (state == QMqttClient::Disconnected)
    {
        qDebug() << "MQTT disconnected";
    }
}
// 发送数据按钮槽函数
void Widget::on_pushButton_clicked()
{
    // 模拟生成环境数据并发布
    corrToQt_t data = generateSampleData();
    QByteArray payload = serializeTotal(data);
    client->publish((QString)PUB, payload);
}

// 生成模拟环境数据
corrToQt_t Widget::generateSampleData()
{
    corrToQt_t data;
    data.enviro.hum = 55.0;
    data.enviro.tem = 22.5;
    data.enviro.sunshine = 300.0;
    data.enviro.NH3 = 0.5;
    data.warn.flag_NH3 = (data.enviro.NH3 > 1.0) ? 1 : 0;
    data.warn.flag_tem = (data.enviro.tem > 30.0) ? 1 : 0;
    data.time = std::time(nullptr);
    data.fans = (data.enviro.tem > 25.0) ? 1 : 0;
    //data.sqlsearch = 0;
    return data;
}
//显示时间
void Widget::updateTime()
{
    // 获取当前时间，并格式化为字符串
    QDateTime currentTime = QDateTime::currentDateTime();
    QString yearMonth = currentTime.toString("yyyy-MM-dd");
    QString hourMinuteSecond = currentTime.toString("HH:mm:ss");
    // 设置字体大小和粗细
    formattedText = QString(
                "<html><head/><body>"
                "<p align='center' style='font-size:16px; font-weight:bold;'>%1</p>"// 年月日，较小字体
                "<p align='center' style='font-size:24px; font-weight:bold;'>%2</p>"// 时分秒，较大字体
                "</body></html>")
            .arg(yearMonth)
            .arg(hourMinuteSecond);
    // 设置QLabel的文本
    ui->lab_time->setText(formattedText);
}
// 将corrToQt_t结构体序列化为JSON格式
QByteArray Widget::serializeTotal(const corrToQt_t &data)
{
    // 将corrToQt_t结构体序列化为JSON格式
    QJsonObject jsonObj;

    QJsonObject envObj;
    envObj["hum"] = data.enviro.hum;  // 湿度
    envObj["tem"] = data.enviro.tem;  // 温度
    envObj["sunshine"] = data.enviro.sunshine;  // 光照强度
    envObj["NH3"] = data.enviro.NH3;  // 氨气浓度

    // 警告标志部分
    QJsonObject warnObj;
    warnObj["flag_NH3"] = data.warn.flag_NH3;  // NH3超标标志
    warnObj["flag_tem"] = data.warn.flag_tem;  // 温度过高标志

    jsonObj["warn"] = warnObj;
    jsonObj["enviro"] = envObj;
    jsonObj["time"] = static_cast<qint64>(data.time);  // 时间戳
    jsonObj["fans"] = data.fans;  // 风扇状态
    jsonObj["sqlsearch"] = data.sqlsearch;
    // 将QJsonObject转换为QByteArray
    QJsonDocument doc(jsonObj);
    return doc.toJson();
}
// 将JSON格式的字节数组反序列化为corrToQt_t结构体
corrToQt_t Widget::deserializeTotal(const QByteArray &jsonData)
{
    QByteArray cleanData = jsonData.trimmed();  // 修剪两端的空白字符
    int nullByteIndex = cleanData.indexOf('\0'); // 查找第一个 '\0' 字节
    if (nullByteIndex != -1) {
        cleanData.truncate(nullByteIndex); // 截断到 '\0' 之前
    }
    // 将JSON格式的字节数组反序列化为corrToQt_t结构体
    corrToQt_t data;
    QJsonDocument doc = QJsonDocument::fromJson(cleanData);
    QJsonObject jsonObj = doc.object();

    // 提取环境数据
    QJsonObject envioObj = jsonObj["enviro"].toObject();
    data.enviro.hum = envioObj["hum"].toDouble();
    data.enviro.tem = envioObj["tem"].toDouble();
    data.enviro.sunshine = envioObj["sunshine"].toDouble();
    data.enviro.NH3 = envioObj["NH3"].toDouble();


    qDebug()<<"hum"<<data.enviro.hum;
    qDebug()<<"temm"<<data.enviro.tem;
    qDebug()<<"sunshine"<<data.enviro.sunshine;
    qDebug()<<"NH3"<<data.enviro.NH3;
    //显示
    ui->label_hum_2->setText(QString::number(data.enviro.hum));
    ui->label_tem_2->setText(QString::number(data.enviro.tem));
    ui->label_sun_2->setText(QString::number(data.enviro.sunshine));
    ui->label_nh3_2->setText(QString::number(data.enviro.NH3));

    // 提取警告标志
    QJsonObject warnObj = jsonObj["warn"].toObject();
    data.warn.flag_NH3 = warnObj["flag_NH3"].toInt();
    data.warn.flag_tem = warnObj["flag_tem"].toInt();
    // 为1生成提示消息
    QStringList messages;

    if (data.warn.flag_NH3)
    {
        messages << "氨气浓度过高";
    }
    if (data.warn.flag_tem)
    {
        messages << "温度浓度过高";
    }

    // 使用换行符连接所有消息
    QString message = messages.join("\n");

    // 将消息设置到文本编辑控件
    ui->textEdit_echo->setText(message);

    // 提取时间戳和风扇状态
    data.time = static_cast<time_t>(jsonObj["time"].toInt());
    data.fans = jsonObj["fans"].toInt();
    data.sqlsearch = jsonObj["sqlsearch"].toInt();

    return data;
}
//创建折线图
void generateLineChart(corrToQt_t *data, int dataCount)
{
    // 创建QLineSeries用于每个参数的折线图
    QLineSeries *humiditySeries = new QLineSeries();
    humiditySeries->setName("Humidity");

    QLineSeries *temperatureSeries = new QLineSeries();
    temperatureSeries->setName("Temperature");

    QLineSeries *sunshineSeries = new QLineSeries();
    sunshineSeries->setName("Sunshine");

    QLineSeries *NH3Series = new QLineSeries();
    NH3Series->setName("NH3 Concentration");

    // 遍历数据并添加到各个QLineSeries中
    for (int i = 0; i < dataCount; i++) {
        QDateTime timestamp;
        timestamp.setSecsSinceEpoch(data[i].time);

        humiditySeries->append(timestamp.toMSecsSinceEpoch(), data[i].enviro.hum);
        temperatureSeries->append(timestamp.toMSecsSinceEpoch(), data[i].enviro.tem);
        sunshineSeries->append(timestamp.toMSecsSinceEpoch(), data[i].enviro.sunshine);
        NH3Series->append(timestamp.toMSecsSinceEpoch(), data[i].enviro.NH3);
    }

    // 创建图表并添加系列
    QChart *chart = new QChart();
    chart->addSeries(humiditySeries);
    chart->addSeries(temperatureSeries);
    chart->addSeries(sunshineSeries);
    chart->addSeries(NH3Series);

    // 创建坐标轴
    QValueAxis *axisX = new QValueAxis;
    axisX->setLabelFormat("%H:%M:%S");  // 设置X轴显示格式为时间
    axisX->setTitleText("Time");
    chart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%.2f");
    axisY->setTitleText("Value");
    chart->addAxis(axisY, Qt::AlignLeft);

    // 将系列与轴关联
    humiditySeries->attachAxis(axisX);
    humiditySeries->attachAxis(axisY);

    temperatureSeries->attachAxis(axisX);
    temperatureSeries->attachAxis(axisY);

    sunshineSeries->attachAxis(axisX);
    sunshineSeries->attachAxis(axisY);

    NH3Series->attachAxis(axisX);
    NH3Series->attachAxis(axisY);

    // 创建图表视图
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // 将图表视图添加到布局或窗口中
    QMainWindow *mainWindow = new QMainWindow();
    mainWindow->setCentralWidget(chartView);
    mainWindow->resize(800, 600);
    mainWindow->show();
}
//搜索按钮
void Widget::on_send_cmd_btn_2_clicked()
{
    corrToQt_t data;

    // 1. 获取QComboBox的当前值
    int value = ui->send_time_for_histoty->currentText().toInt();

    // 2. 将值存入结构体的 sqlsearch 成员
    data.sqlsearch = value;
    // 2. 创建JSON对象
//    QJsonObject jsonObj;
//    jsonObj["sqlsearch"] = data.sqlsearch;

//    QJsonDocument jsonDoc(jsonObj);
//    QByteArray jsonData = jsonDoc.toJson();
    QByteArray jsonData = serializeTotal(data);
    // 3. 将 QByteArray 转换为 char*
    const char* jsonCharData = jsonData.constData();

    // 4发送JSON数据到MQTT主题
    QString topic = (QString)PUB;
    client->publish(topic, jsonCharData);

    // 打印发送的JSON数据
    qDebug() << "Sent JSON:" << jsonCharData;
}

void Widget::on_send_cmd_btn_clicked()
{
    client->connectToHost();
}
