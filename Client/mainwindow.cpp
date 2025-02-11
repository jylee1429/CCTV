#include "mainwindow.h"
#include <QApplication>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), pipeline(nullptr), busThread(nullptr), streamingFlag(false), connectFlag(false), bus(nullptr) {

    ui->setupUi(this);

    videoWidget = ui->displayWidget;
    // 버튼 설정
    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::startStreaming);
    connect(ui->stopButton, &QPushButton::clicked, this, &MainWindow::stopStreaming);

    // 서버 연결
    connectServer();
}

MainWindow::~MainWindow() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        g_source_remove(busWatchId);
        pipeline = nullptr;
    }
    delete ui;
}

void MainWindow::connectServer() {
    if (pipeline) {
        qDebug() <<  "Pipeline already exist";
        return;
    }
    if (streamingFlag) {
        qDebug() << "Streaming is already in progress";
        return;
    }
    QString rtspUrl = "rtsp://61.253.4.180:8888/cctv";
    QString pipelineStr = QString(
                              "rtspsrc location=%1 latency=0 ! rtph264depay ! h264parse ! avdec_h264 ! "
                              "timeoverlay name=timeoverlay show-times-as-dates=true datetime-format='%Y-%m-%d %H:%M:%S' "
                              "halignment=right valignment=top shaded-background=true font-desc='Sans, 5' "
                              "! videoconvert ! glimagesink sync=false"
                              ).arg(rtspUrl);
    // 파이프 라인 설정
    pipeline = gst_parse_launch(pipelineStr.toUtf8().constData(), nullptr);
    if (!pipeline) {
        qDebug() << "Failed to create GStreamer pipeline!";
        return;
    }

    if (!videoWidget) {
        qDebug() << "Error : videoWidget is nullptr";
        return;
    }

    GstElement *videoSink = gst_bin_get_by_interface(GST_BIN(pipeline), GST_TYPE_VIDEO_OVERLAY);
    if (!videoSink) {
        qDebug() << "Failed to get video overlay element";
        return;
    }

    WId winId = videoWidget->winId();
    if (!winId) {
        qDebug() << "Error: winId() returned 0! Streaming cannot start";
    }
    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(videoSink), winId);
    gst_object_unref(videoSink);

    setTimeOverlayEpoch();
    connectFlag = true;

    setMessageBus();
}

void MainWindow::startStreaming() {
    if (streamingFlag) {
        qDebug() << "Streaming is already in progress";
        return;
    }
    if (!pipeline) {
        qDebug() << "Pipeline does not exist";
        return;
    }
    if (!connectFlag) {
        qDebug() << "Server not connected";
        return;
    }
    // start streaming
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    streamingFlag = true;

    qDebug() << "Start playing streaming";
}

void MainWindow::setMessageBus() {
    // 메세지 버스 설정
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    busWatchId = gst_bus_add_watch(bus, handleBusMessage, nullptr);
    gst_object_unref(bus);
    // 메인 루프 실행
    busThread = QThread::create(handleThread);
    busThread->start();
    qDebug() << "Thread start";
}

void MainWindow::handleThread() {
    qDebug() << "handleThread function";
    GMainLoop *loop = g_main_loop_new(NULL, false);
    g_main_loop_run(loop);
}

gboolean MainWindow::handleBusMessage(GstBus *bus, GstMessage *msg, gpointer data) {
    GError *err;
    gchar *debug;

    switch(GST_MESSAGE_TYPE(msg)){
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &err, &debug);
            qDebug() << "Error received from element " << GST_OBJECT_NAME(msg->src) << " : " << err->message;
            g_clear_error(&err);
            g_free(debug);
            qDebug() << "Error occured";
            // showMessageBox("Error", "에러가 발생하였습니다");
            // streamingFlag = false;
            break;
        case GST_MESSAGE_STATE_CHANGED:
            break;
        case GST_MESSAGE_EOS:
            qDebug() << "End of stream reached\nExiting program";
            QCoreApplication::exit(0);
            break;
        default:
            // qDebug() << "Unexpected message received";
            break;
    }

    return true;
}

void MainWindow::stopStreaming() {
    if (!pipeline) {
        qDebug() << "Error : No pipeline exists. Cannot stop streaming";
        return;
    }

    if (!streamingFlag) {
        qDebug() << "Warning : Streaming is already stopped";
        return;
    }

    gst_element_set_state(pipeline, GST_STATE_PAUSED);
    streamingFlag = false;
    qDebug() << "Pause playing streaming";
}

void MainWindow::setTimeOverlayEpoch() {
    if (!pipeline) {
        qDebug() << "Error : Pipeline is NULL";
        return;
    }

    GstElement *overlay = gst_bin_get_by_name(GST_BIN(pipeline), "timeoverlay");
    if (overlay) {
        GDateTime *epoch = g_date_time_new_now_local();
        g_object_set(overlay, "datetime-epoch", epoch, nullptr);
        g_date_time_unref(epoch);
        gst_object_unref(overlay);
    }
    else {
        qDebug() << "Error : timeoverlay element not found!";
    }
}

void MainWindow::showMessageBox(QString title, QString message) {
    QMetaObject::invokeMethod(this, [=]() {
        QMessageBox::information(this, title, message);
    }, Qt::QueuedConnection);
}
