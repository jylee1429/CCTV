#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QDebug>
#include <QThread>
#include <QMessageBox>
#include <QMetaObject>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include "ui_mainwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    static gboolean handleBusMessage(GstBus *bus, GstMessage *msg, gpointer data);
    static void handleThread();
    void showMessageBox(QString title, QString message);
    void setTimeOverlayEpoch();
    void setMessageBus();
private slots:
    void startStreaming();
    void stopStreaming();
    void connectServer();
private:
    Ui::MainWindow *ui;
    QWidget *videoWidget;
    GstElement *pipeline;
    GstBus *bus;
    QThread *busThread;
    QString currentDataTime;
    guint busWatchId;
    bool streamingFlag;
    bool connectFlag;
};
#endif // MAINWINDOW_H
