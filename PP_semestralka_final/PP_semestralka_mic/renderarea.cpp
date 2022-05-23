#include "renderarea.h"
#include <QMessageBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QNetworkAccessManager>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <math.h>
#include <QDebug>
#include <QStyle>



/*render area dimension*/
int renderWidth = 800;
int renderHeight = 400;

int memr = 1;
std::string vystupy[4] = { "u1","u2", "u3", "u4", };
double hodnoty[4];
const QUrl buttonRefs[4] =  {QUrl("http://localhost:8008/api/data/mic/MP_NUDGE:BSTATE?data&mime=application/json") ,
						QUrl("http://localhost:8008/api/data/mic/CNB_DISTRB:YCN?data&mime=application/json") , 
						QUrl("http://localhost:8008/api/data/mic/MP_INTEG_RST:BSTATE?data&mime=application/json") , 
						QUrl("http://localhost:8008/api/data/mic/CNR_y0:ycn?data&mime=application/json")};
const QUrl dataUrl = QUrl("http://localhost:8008/api/data/mic/TRND?data&mime=application/json");
bool connected = true;
bool inprocess = false;



RenderArea::RenderArea(QWidget *parent)
	: QWidget(parent)
{
	
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);

	int width = 2;
	Qt::PenStyle style = Qt::PenStyle(Qt::SolidLine);
	Qt::PenCapStyle cap = Qt::PenCapStyle(Qt::FlatCap);
	Qt::PenJoinStyle join = Qt::PenJoinStyle(Qt::MiterJoin);

	pen = QPen(Qt::black, width, style, cap, join);

	connect(&timer, &QTimer::timeout, this, &RenderArea::tick);
	timer.start(50);

	networkManager = new QNetworkAccessManager(this);

	/*Definice hlavniho layoutu pro data a tlacitka*/
	QWidget* layoutWidget = new QWidget(this);
	QVBoxLayout* hb = new QVBoxLayout(layoutWidget);
	layoutWidget->setGeometry(renderWidth / 2, 0, renderWidth / 2, renderHeight);
	/*pridani formulare do hlavniho layoutu*/
	QFormLayout* form = new QFormLayout();
	form->setHorizontalSpacing(10);
	form->setVerticalSpacing(renderHeight/20);
	hb->addLayout(form);
	/*Vypis hodnot ze simulace*/
	stav = new QLabel(this);
	stav->setText(QString("<b>NEPRIPOJENO</b>"));//defaultni nastaveni
	form->addRow("Stav pripojeni k serveru: ",stav);
	valueFi1 = new QLabel(this);
	form->addRow("Aktualni natoceni spulky [rad]:", valueFi1);
	valuedFi1 = new QLabel(this);
	form->addRow("Aktualni rychlost spulky [rad/s]:", valuedFi1);
	valueFi2 = new QLabel(this);
	form->addRow("Aktualni poloha mice vuci stredu spulky [rad]:", valueFi2);
	valuedFi2 = new QLabel(this);
	form->addRow("Aktualni rychlost mice vuci stredu spulky [rad/s]:", valuedFi2);
	form->setContentsMargins(0, 0, 0, renderHeight / 10);
	/*pridani layoutu pro uzivatelska tlacitka*/
	QGridLayout* layout = new QGridLayout();
	hb->addLayout(layout);
	layout->setContentsMargins(renderWidth / 10, 0, renderWidth / 10, 0);
	/*uzivatelska tlacitka*/
	QPushButton* setNewSetPointBtn = new QPushButton("Stouch do mice", this);
	layout->addWidget(setNewSetPointBtn);
	connect(setNewSetPointBtn, &QPushButton::clicked, this, &RenderArea::handleStouchBtn);
	QPushButton* nahodnaPorucha = new QPushButton("Generovani nah. poruchy", this);
	nahodnaPorucha->setCheckable(true);
	/*pokusy o zmenu barev tlacitek*/
	//nahodnaPorucha->setStyleSheet("QPushButton:clicked {background-color: red;border-radius: 10px;height:30px;}");
	//nahodnaPorucha->setStyleSheet("QPushButton:clicked {background - color: blue;border - radius: 10px;padding: 6px;}");
	/*define buttons
	QPushButton{
		"background-color: red;"
		"border-style: outset;"
		"border-width: 2px;"
		"border-radius: 10px;"
		"border-color: beige;"
		"font: bold 14px;"
		"min-width: 10em;"
		"padding: 6px;"
	};
	QPushButton::clicked{
		"background - color: rgb(224, 0, 0);"
		"border - style: inset;"
	};*/
	layout->addWidget(nahodnaPorucha);
	connect(nahodnaPorucha, &QPushButton::clicked, this, &RenderArea::handlePoruchaBtn);
	QPushButton* resetModelu = new QPushButton("Reset Modelu", this);
	layout->addWidget(resetModelu);
	connect(resetModelu, &QPushButton::clicked, this, &RenderArea::handleResetBtn);
	/*pridani formulare s jednou hodnotou do hlavniho layoutu*/
	QFormLayout* form1 = new QFormLayout();
	form1->setHorizontalSpacing(10);
	form1->setVerticalSpacing(renderHeight / 20);
	form1->setContentsMargins(0, renderHeight / 15, 0, 0);
	hb->addLayout(form1);
	inputBox = new QDoubleSpinBox(this);
	form1->addRow("Pocatecni podminky mice [rad]:", inputBox);
	/*pridani layoutu pro jedno tlacitko do hlavniho layoutu*/
	QGridLayout* layout1 = new QGridLayout();
	hb->addLayout(layout1);
	QPushButton* setPocPodm = new QPushButton("Nastav nove poc. podminky", this);
	layout1->addWidget(setPocPodm,0,0);
	layout1->setContentsMargins(renderWidth / 10,0, renderWidth / 10,0);
	connect(setPocPodm, &QPushButton::clicked, this, &RenderArea::handleSetPocPodmBtn);

	/*mazani nepotrebnych souboru z pameti*/
	//networkManager->setAutoDeleteReplies(true);
	//connect(networkManager, &QNetworkAccessManager::encrypted, this, &RenderArea::deleteLater);
	
	
}

QSize RenderArea::minimumSizeHint() const
{
	return QSize(100, 100);
}

QSize RenderArea::sizeHint() const
{
	return QSize(renderWidth, renderHeight);
}

/*Connection to REST*/
QNetworkRequest RenderArea::getRequest(QUrl url)
{
	QNetworkRequest request = QNetworkRequest(url);

	// HTTP Basic authentication header value: base64(username:password)
	QString username = "admin";
	QString password = "";
	QString concatenated = username + ":" + password;
	QByteArray data = concatenated.toLocal8Bit().toBase64();
	QString headerData = "Basic " + data;
	request.setRawHeader("Authorization", headerData.toLocal8Bit());
	request.setRawHeader("Content-Type", "application/json");

	QSslConfiguration conf = request.sslConfiguration();
	conf.setPeerVerifyMode(QSslSocket::VerifyNone);
	request.setSslConfiguration(conf);
	return request;
}

void RenderArea::requestData()
{
	QUrl url = dataUrl;
	QNetworkRequest request = getRequest(url);

	reply = networkManager->get(request);
	//connect(reply, &QNetworkReply::encrypted, this, &RenderArea::deleteLater);
	connect(reply, &QNetworkReply::finished, this, &RenderArea::onUserDataFetched);
	connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
		this, &RenderArea::onNetworkError);

}

/*obsluzne funkce pro tlacitka*/
void RenderArea::handleStouchBtn()
{
	if (!connected || inprocess)
	{
		return;
	}
	inprocess = true;
	QNetworkRequest request = getRequest(buttonRefs[0]);
	int sp = 1;
	QString payload = "{ \"v\":";
	payload += QString::number(sp);
	payload += " }";
	postreply = networkManager->post(request, payload.toUtf8());
	connect(postreply, &QNetworkReply::finished, this, &RenderArea::finishedPost);
}
void RenderArea::handlePoruchaBtn()
{
	if (!connected || inprocess)
	{
		return;
	}
	inprocess = true;
	QNetworkRequest request = getRequest(buttonRefs[1]);
	QString payload = "{ \"v\":";
	payload += QString::number(memr);
	payload += " }";
	if (memr == 0)
	{
		memr = 1;
	}
	else
	{
		memr = 0;
	}

	postreply = networkManager->post(request, payload.toUtf8());
	connect(postreply, &QNetworkReply::finished, this, &RenderArea::finishedPost);
}
void RenderArea::handleResetBtn()
{
	if (!connected || inprocess)
	{
		return;
	}
	inprocess = true;
	QNetworkRequest request = getRequest(buttonRefs[2]);
	int sp = 1;
	QString payload = "{ \"v\":";
	payload += QString::number(sp);
	payload += " }";

	postreply = networkManager->post(request, payload.toUtf8());
	connect(postreply, &QNetworkReply::finished, this, &RenderArea::finishedPost);
}
void RenderArea::handleSetPocPodmBtn()
{
	if (!connected || inprocess)
	{
		return;
	}
	inprocess = true;
	QNetworkRequest request = getRequest(buttonRefs[3]);
	double sp = inputBox->value();
	qDebug() << inputBox->value();
	QString payload = "{ \"v\":";
	payload += QString::number(sp);
	payload += " }";

	postreply = networkManager->post(request, payload.toUtf8());
	connect(postreply, &QNetworkReply::finished, this, &RenderArea::finishedPost);
}
void RenderArea::finishedPost()
{
	inprocess = false;
	postreply->deleteLater();
}

void RenderArea::onUserDataFetched()
{
	reply = (QNetworkReply*)QObject::sender();
	if (reply->error() == 0)
	{
		connected = true;
		stav->setText(QString("<b>PRIPOJENO</b>"));
	}
	else
	{
		reply->deleteLater();
		return;
	}
	QByteArray buff = reply->readAll();
	reply->deleteLater();

	QJsonDocument jDoc = QJsonDocument::fromJson(buff);
	QJsonObject jResponse = jDoc.object();
	QJsonArray subitems = jResponse["subitems"].toArray();

	for (int i = 0; i < 4; i++)
	{
		QJsonObject data1 = subitems[i].toObject();
		QJsonObject data = data1.value(vystupy[i].c_str()).toObject();
		if (data.value("v").isDouble())
		{
			double value = data.value("v").toDouble();
			hodnoty[i] = value;
		}
	}
	valueFi1->setText(QString::number(hodnoty[0]));
	valuedFi1->setText(QString::number(hodnoty[1]));
	valueFi2->setText(QString::number(hodnoty[2]));
	valuedFi2->setText(QString::number(hodnoty[3]));
	
 }

void RenderArea::onNetworkError(QNetworkReply::NetworkError a_Code)
{
	if (connected)
	{
		connected = false;
		stav->setText(QString("<b>NEPRIPOJENO</b>"));
		qDebug() << a_Code;
		QNetworkReply* errReply = (QNetworkReply*)QObject::sender();
		QString error = "Network related error (error code: " + QString::number((int)a_Code)+")"
			" Unable to communicate with the server (" + errReply->errorString() + ") . Please connect to server and try again later.";
		QMessageBox::information(this, "Network error", error);
	}
}


/*Graphics in app*/
void RenderArea::paintEvent(QPaintEvent* /* event */)
{
	/*
	QPainterPath zakladna;
	int zakladnaX = 50;
	int zakladnaY = 200;
	int sirkaZakladny = 150;
	zakladna.addRect(zakladnaX,zakladnaY,sirkaZakladny,sirkaZakladny);

	QPainterPath civka;
	civka.addEllipse(zakladnaX, zakladnaY-(sirkaZakladny/2), sirkaZakladny, sirkaZakladny);

	//priklad
	static const QPoint points[4] = {
		QPoint(10, 80),
		QPoint(20, 10),
		QPoint(80, 30),
		QPoint(90, 70)
	};

	QRect rect(10, 20, 80, 60);

	QPainterPath path;
	path.moveTo(20, 80);
	path.lineTo(20, 30);
	path.cubicTo(80, 0, 50, 50, 80, 80);

	int startAngle = 20 * 16;
	int arcLength = 120 * 16;

	QPainter painter(this);
	painter.setPen(pen);
	painter.setBrush(Qt::lightGray);
	painter.setRenderHint(QPainter::Antialiasing, true);

	painter.drawPath(zakladna);
	painter.setBrush(Qt::black);
	painter.drawPath(civka);
	painter.save();
	painter.drawEllipse(zakladnaX, zakladnaY - (sirkaZakladny / 2), sirkaZakladny / 2, sirkaZakladny / 2);
	painter.rotate(180);

	//painter.translate(100, 100);

	// painter.drawLine(rect.bottomLeft(), rect.topRight());
	 //painter.drawPoints(points, 4);
	// painter.drawPolyline(points, 4);
	// painter.drawPolygon(points, 4);
	//painter.drawRect(rect);
	// painter.drawRoundedRect(rect, 25, 25, Qt::RelativeSize);
	//painter.drawEllipse(rect);
	// painter.drawArc(rect, startAngle, arcLength);
	 painter.drawChord(rect, startAngle, arcLength);
	// painter.drawPie(rect, startAngle, arcLength);
	// painter.drawPath(path);
	 /*
	 painter.drawText(rect,
						 Qt::AlignCenter,
						 tr("Qt by\nThe Qt Company"));

	painter.restore();

	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setPen(palette().dark().color());
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(QRect(0, 0, width() - 1, height() - 1));
	painter.save();
	*/

	//nastaveni painteru
	QPainter painter(this);
	painter.setPen(pen);
	painter.setBrush(Qt::lightGray);
	painter.setRenderHint(QPainter::Antialiasing, true);

	//vykresleni nepohyblive zakladny
	int zakladnaX = 100;
	int zakladnaY = 240;
	int sirkaZakladny = 150;
	painter.drawRect(zakladnaX, zakladnaY, sirkaZakladny, sirkaZakladny);
	QPainterPath operkaR;
	operkaR.moveTo(zakladnaX + sirkaZakladny, zakladnaY);
	operkaR.lineTo(zakladnaX+sirkaZakladny*1.5,zakladnaY);
	operkaR.lineTo(zakladnaX + sirkaZakladny * 1.5, zakladnaY - sirkaZakladny * 0.5);
	operkaR.lineTo(zakladnaX + sirkaZakladny * 1.4, zakladnaY - sirkaZakladny * 0.5);
	operkaR.lineTo(zakladnaX + sirkaZakladny * 1.4, zakladnaY - sirkaZakladny * 0.1);
	operkaR.lineTo(zakladnaX + sirkaZakladny, zakladnaY - sirkaZakladny * 0.1);
	operkaR.lineTo(zakladnaX + sirkaZakladny*0.95, zakladnaY);
	painter.drawPath(operkaR);
	QPainterPath operkaL;
	operkaL.moveTo(zakladnaX, zakladnaY);
	operkaL.lineTo(zakladnaX - sirkaZakladny * 0.5, zakladnaY);
	operkaL.lineTo(zakladnaX - sirkaZakladny * 0.5, zakladnaY - sirkaZakladny * 0.5);
	operkaL.lineTo(zakladnaX - sirkaZakladny * 0.4, zakladnaY - sirkaZakladny * 0.5);
	operkaL.lineTo(zakladnaX - sirkaZakladny * 0.4, zakladnaY - sirkaZakladny * 0.1);
	operkaL.lineTo(zakladnaX, zakladnaY - sirkaZakladny * 0.1);
	operkaL.lineTo(zakladnaX + sirkaZakladny * 0.05, zakladnaY);
	painter.drawPath(operkaL);
	painter.drawEllipse(QPoint(zakladnaX - sirkaZakladny * 0.45, zakladnaY - sirkaZakladny * 0.5), 15, 15);
	painter.drawEllipse(QPoint(zakladnaX + sirkaZakladny * 1.45, zakladnaY - sirkaZakladny * 0.5), 15, 15);
	
	//vykresleni civky
	painter.setBrush(Qt::black);
	painter.drawEllipse(zakladnaX, zakladnaY - (sirkaZakladny / 2), sirkaZakladny, sirkaZakladny);
	painter.setBrush(Qt::white);
	double angle = (180 / M_PI) * (valueFi1->text().toDouble());
	painter.drawPie(QRectF(zakladnaX, zakladnaY - (sirkaZakladny / 2), sirkaZakladny, sirkaZakladny), (angle) * 16, 30 * 16);
	painter.drawPie(QRectF(zakladnaX, zakladnaY - (sirkaZakladny / 2), sirkaZakladny, sirkaZakladny), (angle+90) * 16, 30 * 16);
	painter.drawPie(QRectF(zakladnaX, zakladnaY - (sirkaZakladny / 2), sirkaZakladny, sirkaZakladny), (angle + 180) * 16, 30 * 16);
	painter.drawPie(QRectF(zakladnaX, zakladnaY - (sirkaZakladny / 2), sirkaZakladny, sirkaZakladny), (angle+270) * 16, 30 * 16);

	//vykresleni mice vcetne rotace jeho samotneho
	painter.translate(QPoint(zakladnaX + sirkaZakladny * 0.5, zakladnaY));
	painter.rotate((180 / M_PI)* (valueFi2->text().toDouble()));
	painter.translate(QPoint(0, -sirkaZakladny));
	painter.rotate((180 / M_PI)* (valueFi1->text().toDouble()));
	QRadialGradient gradient(0, -sirkaZakladny * 0.4, sirkaZakladny*0.4, 0, -sirkaZakladny*0.2, sirkaZakladny*0.5);
	gradient.setColorAt(0, Qt::yellow);
	gradient.setColorAt(1, Qt::green);
	QBrush brush(gradient);
	painter.setBrush(brush);
	painter.drawEllipse(QPoint(0, 0), sirkaZakladny / 2, sirkaZakladny / 2);
	
}

void RenderArea::tick()
{
	requestData();
	update();
}
