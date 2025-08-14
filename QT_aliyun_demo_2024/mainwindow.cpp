#include "mainwindow.h"
#include "ui_mainwindow.h"

// JSON 字符串，消息载荷。
QString aa = "\"/sys/k1own7VPzJY/QT_dev/thing/service/property/set\" payload: \"{\"deviceType\":\"CustomCategory\",\"iotId\":\"2JIEMH9lq6mV9IiMrkZh000000\",\"requestId\":\"null\",\"checkFailedData\":{},\"productKey\":\"a1MiW5W8ufE\",\"gmtCreate\":1690348115173,\"deviceName\":\"stm32\",\"items\":{\"tu_d\":{\"time\":1690348115150,\"value\":20},\"tu\":{\"time\":1690348115150,\"value\":38},\"temp\":{\"time\":1690348115150,\"value\":30},\"humi_d\":{\"time\":1690348115150,\"value\":30},\"co2\":{\"time\":1690348115150,\"value\":0},\"humi\":{\"time\":1690348115150,\"value\":55},\"lux_d\":{\"time\":1690348115150,\"value\":1000},\"co2_d\":{\"time\":1690348115150,\"value\":60},\"temp_d\":{\"time\":1690348115150,\"value\":20},\"temp_u\":{\"time\":1690348115150,\"value\":30},\"lux\":{\"time\":1690348115150,\"value\":284}}}\"";
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    clientMqtt();// 调用 MQTT 客户端初始化函数。
}

MainWindow::~MainWindow()
{
    delete ui;
}
// 定义MQTT 客户端初始化函数。
void MainWindow::clientMqtt(){
    client = new QMQTT::Client(); // 初始化QMQTT客户指针

    QHostAddress host("k1own7VPzJY.iot-as-mqtt.cn-shanghai.aliyuncs.com"); // MQTT代理服务器 IP
    QByteArray password = "56a081262deacf3d283dd3e567468ea4f56318d1d3a002f732e5b7084442017b"; // 密码，1改成自己的。
    quint16 port = 1883; // 代理服务器端口
    QString deviceId = "k1own7VPzJY.QT_dev|securemode=2,signmethod=hmacsha256,timestamp=1725183217887|"; // 连接 ID，2改成自己的。
    QString productId = "QT_dev&k1own7VPzJY";   //连接名称，3ProductKey改成自己的。

    client->setKeepAlive(120); // 设置 MQTT 保持活动间隔。心跳
    client->setHostName("k1own7VPzJY.iot-as-mqtt.cn-shanghai.aliyuncs.com");//4ProductKey改成自己的。
    client->setPort(port); // 设置 EMQ 代理服务器端口
    client->setClientId(deviceId); // 设备 ID
    client->setUsername(productId); // 产品 ID
    client->setPassword(password);
    client->cleanSession();
    client->setVersion(QMQTT::MQTTVersion::V3_1_1); // 设置mqtt版本

    client->connectToHost(); // 连接 EMQ 代理服务器
    client->subscribe("/k1own7VPzJY/QT_dev/user/get");//5ProductKey改成自己的。

    connect(client,&QMQTT::Client::received,this,&MainWindow::onMQTT_Received);

}
// 定义MQTT 客户端接收函数。
void MainWindow::onMQTT_Received(QMQTT::Message message) {
    QString jsonStr = message.payload();
    qDebug() <<jsonStr;

        // 假设您有一个名为 jsonStr 的QString变量，其中包含了您提供的JSON数据
            //QString jsonStr = "{\"deviceType\":\"CustomCategory\",\"iotId\":\"h3qnOaUBv5Qh7JidgnxG000000\",\"requestId\":\"123\",\"checkFailedData\":{},\"productKey\":\"k1own7VPzJY\",\"gmtCreate\":1697554985343,\"deviceName\":\"NB_dev\",\"items\":{\"temp\":{\"time\":1697554985339,\"value\":30},\"humi\":{\"time\":1697554985339,\"value\":65}}}";
            qDebug()<<"AAAAAAAAAA";
            // 将JSON字符串解析为QJsonDocument
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());

            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();

                // 检查是否存在 "items" 键并且它是一个对象
                if (obj.contains("items") && obj["items"].isObject()) {
                    QJsonObject itemsObj = obj["items"].toObject();

                    // 获取 "temp" 对象中的 "value" 值
                    if (itemsObj.contains("temp") && itemsObj["temp"].isObject()) {
                        QJsonObject tempObj = itemsObj["temp"].toObject();
                        float tempValue = tempObj["value"].toDouble();
                        // 现在 tempValue 包含了 "temp" 的值
                        qDebug() << "提取出了:" << tempValue;
                        ui->textEdit_2->setText(QString::number(tempValue)+"℃");
                    }
                    // 获取 "humi" 对象中的 "value" 值
                    if (itemsObj.contains("humi") && itemsObj["humi"].isObject()) {
                        QJsonObject humiObj = itemsObj["humi"].toObject();
                        int humiValue = humiObj["value"].toInt();
                        // 现在 humiValue 包含了 "humi" 的值
                        qDebug() << "提取出了:" << humiValue;
                        ui->textEdit->setText(QString::number(humiValue) + "%");
                    }
                }
            }

}

void MainWindow::onMQTT_subscribed(QString topic, quint8 qos) {
    qDebug() << "\n订阅topic = " << topic << "Qos = " << qos << " 成功!";
}

void MainWindow::onMQTT_unSubscribed(QString topic) {
    qDebug() << "\nonMQTT_unSubscribed:" << topic;
}
// 将数据发布到 MQTT 主题。
void MainWindow::sendTopic(QString data){
    client->publish(QMQTT::Message(136,"/k1own7VPzJY/QT_dev/user/update",data.toUtf8()));//7ProductKey改成自己的。
}
// 按钮实现类似的按钮点击事件处理程序。
void MainWindow::on_pushButton_clicked()
{    
    // 处理按钮点击事件。
    static char relay_f = 0;
    QString fas;
    if(relay_f==0)
    {
        relay_f = 1;
        sendTopic("{\"relay_f\":1}");
    }
    else
    {
        relay_f = 0;
        sendTopic("{\"relay_f\":0}");
    }       
}
// 为其他按钮实现类似的按钮点击事件处理程序。
void MainWindow::on_pushButton_2_clicked()
{
    // 处理按钮点击事件。
    static char aircleaner_f = 0;
    QString fas;
    if(aircleaner_f==0)
    {
        aircleaner_f = 1;
        sendTopic("{\"aircleaner_f\":1}");
    }
    else
    {
        aircleaner_f = 0;
        sendTopic("{\"aircleaner_f\":0}");
    }
}
// 为其他按钮实现类似的按钮点击事件处理程序。
void MainWindow::on_pushButton_3_clicked()
{
    // 处理按钮点击事件。
    static char AC_f = 0;
    QString fas;
    if(AC_f==0)
    {
        AC_f = 1;
        sendTopic("{\"AC_f\":1}");
    }
    else
    {
        AC_f = 0;
        sendTopic("{\"AC_f\":0}");
    }

}

// 为其他按钮实现类似的按钮点击事件处理程序。
