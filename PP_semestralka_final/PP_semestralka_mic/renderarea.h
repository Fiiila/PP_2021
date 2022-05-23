#ifndef RENDERAREA_H
#define RENDERAREA_H

#include <QtGui>
#include <QBrush>
#include <QPen>
#include <QPixmap>
#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QDoubleSpinBox>

#include <QNetworkAccessManager>
#include <QNetworkReply>

class RenderArea : public QWidget
{
    Q_OBJECT

public:
    RenderArea(QWidget *parent = 0);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPen pen;
	QTimer timer;
    QNetworkAccessManager* networkManager;
    QNetworkReply* reply;
    QNetworkReply* postreply;

    QLabel* stav;
    QLabel* valueFi1;
    QLabel* valuedFi1;
    QLabel* valueFi2;
    QLabel* valuedFi2;
    QDoubleSpinBox* inputBox;

    QNetworkRequest getRequest(QUrl url);

private slots:
	void tick();
    void requestData();
    void handleStouchBtn();
    void handlePoruchaBtn();
    void handleResetBtn();
    void handleSetPocPodmBtn();
    void onUserDataFetched();
    void onNetworkError(QNetworkReply::NetworkError a_Code);
    void finishedPost();
};

#endif // RENDERAREA_H
