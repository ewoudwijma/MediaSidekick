#ifndef MEXPORTDIALOG_H
#define MEXPORTDIALOG_H

#include "agprocessthread.h"
#include "agviewrectitem.h"
#include "aglobal.h"
#include "agcliprectitem.h"

#include <QAbstractButton>
#include <QDialog>
#include <QGraphicsScene>

namespace Ui {
class MExportDialog;
}

class MExportDialog : public QDialog
{
    Q_OBJECT

    QString transitionValueChangedBy;
    QList<MTimelineGroupRectItem *> timelineGroupList;
    QString fileNameWithoutExtension;

public:
    explicit MExportDialog(QWidget *parent = nullptr, AGViewRectItem *rootItem = nullptr);
    ~MExportDialog();

    QList<AGProcessAndThread *> *processes;

    AGViewRectItem *rootItem;

private slots:
    void onProcessOutput(QTime time, QTime totalTime, QString event, QString outputString);

    void on_exportSizeComboBox_currentTextChanged(const QString &arg1);

    void on_exportFramerateComboBox_currentTextChanged(const QString &arg1);

    void on_clipsFramerateComboBox_currentTextChanged(const QString &arg1);

    void on_exportButton_clicked();

    void on_watermarkButton_clicked();

    void on_exportTargetComboBox_currentTextChanged(const QString &arg1);

    void on_transitionDurationTimeEdit_timeChanged(const QTime &time);

    void on_transitionComboBox_currentTextChanged(const QString &arg1);

    void on_clipsSizeComboBox_currentTextChanged(const QString &arg1);

    void on_transitionDial_valueChanged(int value);
private:
    Ui::MExportDialog *ui;
    void watermarkFileNameChanged(QString newFileName);

    void muxVideoAndAudio();
    void losslessVideoAndAudio();
    void encodeVideoClips();

    int exportWidth;
    int exportHeight;
    int exportFramerate;
    QString exportSizeShortName;
    void loadSettings();
    void changeUIProperties();
    void allTooltips();
    void exportShotcut();
    void exportPremiere();

    QTextStream stream;

    void s(QString y, QString arg1 = "", QString arg2 ="", QString arg3="", QString arg4="");
    void addPremiereClipitem(AGClipRectItem *clipItem, QString clipId, QFileInfo fileInfo, int startTime, int endTime, int inTime, int outTime, QString frameRate, QString mediaType, QMap<QString, FileStruct> *filesMap, int channelTrackNr);
    void addPremiereTransitionItem(int startTime, int endTime, QString frameRate, QString mediaType, QString startOrEnd);
    void addPremiereTrack(QString mediaType, MTimelineGroupRectItem *timelineItem, QMap<QString, FileStruct> filesMap);

signals:
    void processOutput(QTime time, QTime totalTime, QString event, QString outputString);
    void arrangeItems();
    void fileWatch(QString folderFileName, bool on, bool triggerFileChanged = false);
};

#endif // MEXPORTDIALOG_H
