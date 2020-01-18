#ifndef AGEOCODING_H
#define AGEOCODING_H

//https://stackoverflow.com/questions/50344495/how-to-get-latitude-longitude-from-geo-address-using-qt-c-qgeoserviceprovider

// standard C++ header:
#include <iostream>
#include <sstream>

// Qt header:
#include <QtWidgets>
#include <QGeoAddress>
#include <QGeoCodingManager>
#include <QGeoCoordinate>
#include <QGeoLocation>
#include <QGeoServiceProvider>
#include <QWidget>
#include <QGeoPositionInfo>

// main window class
class AGeocoding: public QWidget {
    Q_OBJECT

  // variables:
  private:
    // list of service providers
    std::vector<QGeoServiceProvider*> pQGeoProviders;
    // Qt widgets (contents of main window)
    QVBoxLayout qBox;
    QFormLayout qForm;
    QLabel qLblCountry;
    QLineEdit qTxtCountry;
    QLabel qLblZipCode;
    QLineEdit qTxtZipCode;
    QLabel qLblCity;
    QLineEdit qTxtCity;
    QLabel qLblStreet;
    QLineEdit qTxtStreet;
    QLabel qLblProvider;
    QComboBox qLstProviders;
    QPushButton qBtnFind;
    QLabel qLblLog;
    QTextEdit qTxtLog;

    QGeoCoordinate currentLocation;
    bool geoPositioning;

  // methods:
  public: // ctor/dtor
    AGeocoding(QWidget *pQParent = nullptr);
    virtual ~AGeocoding();
    AGeocoding(const AGeocoding&) = delete;
    AGeocoding& operator=(const AGeocoding&) = delete;

  private: // internal stuff
    void init(); // initializes geo service providers
    void find(); // sends request
    void report(); // processes reply
    void log(const QString &qString)
    {
      qTxtLog.setPlainText(qTxtLog.toPlainText() + qString);
      qTxtLog.moveCursor(QTextCursor::End);
    }
    void log(const char *text) { log(QString::fromUtf8(text)); }
    void log(const std::string &text) { log(text.c_str()); }

  signals:
    void finished(QGeoCoordinate geoCoord);
private slots:
    void positionUpdated(const QGeoPositionInfo &info);
};


#endif // AGEOCODING_H
