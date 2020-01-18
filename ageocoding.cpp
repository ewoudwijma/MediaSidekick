#include "ageocoding.h"

#include <QDebug>
#include <QGeoPositionInfoSource>

AGeocoding::AGeocoding(QWidget *pQParent):
  QWidget(pQParent),
  qLblCountry(QString::fromUtf8("Country:")),
  qLblZipCode(QString::fromUtf8("Postal Code:")),
  qLblCity(QString::fromUtf8("City:")),
  qLblStreet(QString::fromUtf8("Street:")),
  qLblProvider(QString::fromUtf8("Provider:")),
  qBtnFind(QString::fromUtf8("Find Coordinates")),
  qLblLog(QString::fromUtf8("Log:"))
{
  // setup child widgets
  qForm.addRow(&qLblCountry, &qTxtCountry);
  qForm.addRow(&qLblZipCode, &qTxtZipCode);
  qForm.addRow(&qLblCity, &qTxtCity);
  qForm.addRow(&qLblStreet, &qTxtStreet);
  qForm.addRow(&qLblProvider, &qLstProviders);
  qBox.addLayout(&qForm);
  qBox.addWidget(&qBtnFind);
  qBox.addWidget(&qLblLog);
  qBox.addWidget(&qTxtLog);
  setLayout(&qBox);
  // init service provider list
  init();
  // install signal handlers
  QObject::connect(&qBtnFind, &QPushButton::clicked,
    this, &AGeocoding::find);
  // fill in a sample request with a known address initially

  qTxtCountry.setText(QSettings().value("Country").toString());
  qTxtZipCode.setText(QSettings().value("PostalCode").toString());
  qTxtCity.setText(QSettings().value("City").toString());
  qTxtStreet.setText(QSettings().value("Street").toString());

  //https://stackoverflow.com/questions/44321056/qt-get-last-known-location-quickly
  geoPositioning = false;
  if (geoPositioning)
  {
      QGeoPositionInfoSource  *source = QGeoPositionInfoSource::createDefaultSource(this);
      if (source) {
             connect(source, SIGNAL(positionUpdated(QGeoPositionInfo)),
                              this, SLOT(positionUpdated(QGeoPositionInfo)));
             source->startUpdates();
             source->setUpdateInterval(60);

         }
  }

//  log("hello world");
}

AGeocoding::~AGeocoding()
{
  // clean-up
  for (QGeoServiceProvider *pQGeoProvider : pQGeoProviders) {
    delete pQGeoProvider;
  }
}

void AGeocoding::positionUpdated(const QGeoPositionInfo &info)
{
    if (geoPositioning)
    {
      if(info.isValid())
      {
          currentLocation = info.coordinate();
          qDebug() << "Current Latitude : " << currentLocation.latitude();
          qDebug() << "Current Longitude : " << currentLocation.longitude();
//          updateDeviceCordinate(currentLocation);
      }
    }
}

void AGeocoding::init()
{
  // initialize Geo Service Providers
  { std::ostringstream out;
    QStringList qGeoSrvList = QStringList() << "esri";
//      = QGeoServiceProvider::availableServiceProviders();
    for (QString entry : qGeoSrvList) {
      out << "Connecting to service: " << entry.toStdString() << '\n';
      // choose provider
      QGeoServiceProvider *pQGeoProvider = new QGeoServiceProvider(entry);
      if (!pQGeoProvider) {
        out
          << "ERROR: GeoServiceProvider '" << entry.toStdString()
          << "' not available!\n";
        continue;
      }
      QGeoCodingManager *pQGeoCoder = pQGeoProvider->geocodingManager();
      if (!pQGeoCoder) {
        out
          << "ERROR: GeoCodingManager '" << entry.toStdString()
          << "' not available!\n";
        delete pQGeoProvider;
        continue;
      }
      QLocale qLocaleC(QLocale::C, QLocale::AnyCountry);
      pQGeoCoder->setLocale(qLocaleC);
      qLstProviders.addItem(entry);
      pQGeoProviders.push_back(pQGeoProvider);
      out << "Service " << entry.toStdString() << " available.\n";
    }
//    log("out.str()");
    log(out.str());
  }
  if (pQGeoProviders.empty()) qBtnFind.setEnabled(false);
}

std::string format(const QGeoAddress &qGeoAddr)
{
  std::ostringstream out;
  out
    << qGeoAddr.country().toUtf8().data() << "; "
    << qGeoAddr.postalCode().toUtf8().data() << "; "
    << qGeoAddr.city().toUtf8().data() << "; "
    << qGeoAddr.street().toUtf8().data();
  return out.str();
}

void AGeocoding::find()
{
  // get current geo service provider
  QGeoServiceProvider *pQGeoProvider
    = pQGeoProviders[qLstProviders.currentIndex()];
  // fill in request
  QGeoAddress qGeoAddr;
  qGeoAddr.setCountry(qTxtCountry.text());
  qGeoAddr.setPostalCode(qTxtZipCode.text());
  qGeoAddr.setCity(qTxtCity.text());
  qGeoAddr.setStreet(qTxtStreet.text());

  QSettings().setValue("Country", qTxtCountry.text());
  QSettings().setValue("PostalCode", qTxtZipCode.text());
  QSettings().setValue("City", qTxtCity.text());
  QSettings().setValue("Street", qTxtStreet.text());
  QSettings().sync();

  QGeoCodeReply *pQGeoCode
    = pQGeoProvider->geocodingManager()->geocode(qGeoAddr);
  if (!pQGeoCode) {
    log("GeoCoding totally failed!\n");
    return;
  }
  { std::ostringstream out;
    out << "Sending request for:\n"
      << format(qGeoAddr) << "...\n";
    log(out.str());
  }
  // install signal handler to process result later
  QObject::connect(pQGeoCode, &QGeoCodeReply::finished,
    this, &AGeocoding::report);
  /* This signal handler will delete it's own sender.
   * Hence, the connection need not to be remembered
   * although it has only a limited life-time.
   */
}

void AGeocoding::report()
{
  QGeoCodeReply *pQGeoCode
    = dynamic_cast<QGeoCodeReply*>(sender());
  // process reply
  std::ostringstream out;
  out << "Reply: " << pQGeoCode->errorString().toStdString() << '\n';
  switch (pQGeoCode->error()) {
    case QGeoCodeReply::NoError: {
      // eval result
      QList<QGeoLocation> qGeoLocs = pQGeoCode->locations();
      out << qGeoLocs.size() << " location(s) returned.\n";
      for (QGeoLocation &qGeoLoc : qGeoLocs) {
        QGeoAddress qGeoAddr = qGeoLoc.address();
        QGeoCoordinate qGeoCoord = qGeoLoc.coordinate();
        out
          << "Coordinates for "
          << qGeoAddr.text().toUtf8().data() << ":\n"
          << "Lat.:  " << qGeoCoord.latitude() << '\n'
          << "Long.: " << qGeoCoord.longitude() << '\n'
          << "Alt.:  " << qGeoCoord.altitude() << '\n';
//        emit finished(qGeoCoord);
      }
    } break;
#define CASE(ERROR) \
case QGeoCodeReply::ERROR: out << #ERROR << '\n'; break
    CASE(EngineNotSetError);
    CASE(CommunicationError);
    CASE(ParseError);
    CASE(UnsupportedOptionError);
    CASE(CombinationError);
    CASE(UnknownError);
#undef CASE
    default: out << "Undocumented error!\n";
  }
  // log result
  log(out.str());
  // clean-up
  /* delete sender in signal handler could be lethal
   * Hence, delete it later...
   */
  pQGeoCode->deleteLater();
  emit finished(pQGeoCode->locations().first().coordinate());
}
