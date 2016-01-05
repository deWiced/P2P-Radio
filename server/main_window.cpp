#include "main_window.h"
#include "ui_main_window.h"


MainWindow::MainWindow(ServerStreamer *s, QWidget *parent) :
    stream(s),
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("P2P Master");

    stream->init();

    connect(stream, &ServerStreamer::clientCountChanged, this, &MainWindow::updateClientCount);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pingCLientsButton_clicked()
{
    stream->sendMessage();
}

void MainWindow::updateClientCount(int count)
{
    ui->numConnections->setText(QString::number(count));
}
