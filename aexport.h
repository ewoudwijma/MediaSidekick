#ifndef MEXPORTDIALOG_H
#define MEXPORTDIALOG_H

#include "agprocessthread.h"
#include "agviewrectitem.h"
#include "aglobal.h"

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
    QList<AGViewRectItem *> timelineGroupList;
    QString fileNameWithoutExtension;

public:
    explicit MExportDialog(QWidget *parent = nullptr, AGViewRectItem *rootItem = nullptr);
    ~MExportDialog();

    QList<AGProcessAndThread *> *processes;

    AGViewRectItem *rootItem;

private slots:
    void onProcessOutput(QTime time, QTime totalTime, QString event, QString outputString);
    void on_exportVideoAudioSlider_valueChanged(int value);

    void on_exportSizeComboBox_currentTextChanged(const QString &arg1);

    void on_exportFramerateComboBox_currentTextChanged(const QString &arg1);

    void on_clipsFramerateComboBox_currentTextChanged(const QString &arg1);

    void on_exportButton_clicked();

    void on_watermarkButton_clicked();

    void on_exportTargetComboBox_currentTextChanged(const QString &arg1);

    void on_transitionTimeSpinBox_valueChanged(int arg1);

    void on_transitionDial_valueChanged(int value);

    void on_transitionDial_sliderMoved(int position);

    void on_transitionComboBox_currentTextChanged(const QString &arg1);

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
    void addPremiereClipitem(QString clipId, QFileInfo fileInfo, int startFrames, int endFrames, int inFrames, int outFrames, QString frameRate, QString mediaType, QMap<QString, FileStruct> *filesMap, int channelTrackNr, QString clipAudioChannels, QString imageWidth, QString imageHeight);
    void addPremiereTransitionItem(int startFrames, int endFrames, QString frameRate, QString mediaType, QString startOrEnd);
    void addPremiereTrack(QString mediaType, AGViewRectItem *timelineItem, QMap<QString, FileStruct> filesMap);
signals:
    void processOutput(QTime time, QTime totalTime, QString event, QString outputString);
};

#endif // MEXPORTDIALOG_H
