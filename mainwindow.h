#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "fstareditor.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QSettings>
#include <QStandardItemModel>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionBlack_theme_triggered();

    void on_actionQuit_triggered();

    void on_actionAbout_triggered();

    void on_actionAbout_Qt_triggered();

    void on_newEditButton_clicked();

    void on_propertyFilterLineEdit_textChanged(const QString &arg1);

    void on_propertyDiffCheckBox_stateChanged(int arg1);

    void onEditFilterChanged();
    void on_actionWhite_theme_triggered();

    void on_allCheckBox_clicked(bool checked);

    void on_action5_stars_triggered();
    void on_action4_stars_triggered();
    void on_action3_stars_triggered();
    void on_action2_stars_triggered();
    void on_action1_star_triggered();
    void on_action0_stars_triggered();
    void on_actionRepeat_triggered();

    void on_actionSave_triggered();

    void on_actionPlay_Pause_triggered();

    void on_actionNew_triggered();

private:
    Ui::MainWindow *ui;
    QStandardItemModel *tagFilter1Model;
    QStandardItemModel *tagFilter2Model;

signals:
    void propertyFilterChanged(QLineEdit *propertyFilterLineEdit, QCheckBox *propertyDiffCheckBox);
    void editFilterChanged(FStarEditor *starEditorFilterWidget, QListView *tagFilter1ListView, QListView *tagFilter2ListView);
    void giveStars(int starCount);
};

#endif // MAINWINDOW_H
