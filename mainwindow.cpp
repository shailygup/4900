#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "takephoto.h"
#include <QTableView>
#include <QItemDelegate>
#include <QStandardItemModel>
#include "payment.h"
#include <QPrinter>
#include <QProgressDialog>
#include <qtrpt.h>
#include "worker.h"
#include "changepassword.h"
#include "duplicateclients.h"

MainWindow::RoomHistoryStruct roomHistStruct;

QLabel *lbl_curUser;
QLabel *lbl_curShift;

//MyModel* checkoutModel;
Report *checkoutReport, *vacancyReport, *lunchReport, *wakeupReport,
    *bookingReport, *transactionReport, *clientLogReport, *otherReport,
    *yellowReport, *redReport;
bool firstTime = true;
QStack<int> backStack;
QStack<int> forwardStack;
int workFlow;

//client search info
int transacNum, transacTotal;
bool newTrans;
int bookingNum, bookingTotal;
bool newHistory;
int maxClient;
QFuture<void> displayFuture ;
QFuture<void> displayPicFuture;
QFuture<void> transacFuture;
QFuture<void> bookHistoryFuture;
bool newReceipt;


//register Type
int registerType;

//CaseFiles stuff
QVector<QTableWidget*> pcp_tables;
QVector<QString> pcpTypes;
bool loaded = false;
QString idDisplayed;

QString transType;
bool isRefund = false;
QStringList curReceipt;

QProgressDialog* dialog;

//QSettings settings(QSettings::IniFormat, QSettings::SystemScope,
//                   "The Salvation Army", "ARCWay");

//QSqlQuery resultssss;
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<QVector<int>>("QVector<int>");
    qRegisterMetaType<QTextBlock>("QTextBlock");
    qRegisterMetaType<QTextCursor>("QTextCursor");
    qRegisterMetaType<Qt::Orientation>("Qt::Orientation");
    ui->setupUi(this);
    setCentralWidget(ui->stackedWidget);
    cNew= false;
    bNew= false;
    tNew= false;
    ui->makeBookingButton->hide();
    dateChanger = false;
    //mw = this;

    //default signal of stackedWidget
    //detect if the widget is changed
    connect(ui->stackedWidget, SIGNAL(currentChanged(int)), this, SLOT(initCurrentWidget(int)));
    connect(dbManager, SIGNAL(dailyReportStatsChanged(QList<int>, bool)), this,
        SLOT(updateDailyReportStats(QList<int>, bool)));
    connect(dbManager, SIGNAL(shiftReportStatsChanged(QStringList, bool)), this,
        SLOT(updateShiftReportStats(QStringList, bool)));
    connect(dbManager, SIGNAL(cashFloatChanged(QDate, int, QStringList, bool)), this,
        SLOT(updateCashFloat(QDate, int, QStringList, bool)));
    connect(dbManager, SIGNAL(cashFloatInserted(QString, QString, QString)), this,
        SLOT(updateCashFloatLastEditedLabels(QString, QString, QString)));
    connect(ui->other_lineEdit, SIGNAL(textEdited(const QString &)), this,
        SLOT(on_other_lineEdit_textEdited(const QString &)));
    connect(dbManager, SIGNAL(monthlyReportChanged(QStringList, bool)), this,
        SLOT(updateMonthlyReportUi(QStringList, bool)));
    connect(dbManager, SIGNAL(noDatabaseConnection(QSqlDatabase*)), this,
        SLOT(on_noDatabaseConnection(QSqlDatabase*)), Qt::DirectConnection);
    connect(dbManager, SIGNAL(noDatabaseConnection()), this,
        SLOT(on_noDatabaseConnection()));
    connect(dbManager, SIGNAL(reconnectedToDatabase()), this,
        SLOT(on_reconnectedToDatabase()));
    connect(dbManager, SIGNAL(getPcpData(QStringList,QStringList,QStringList,int,bool)), this,
        SLOT(populatePcpTable(QStringList, QStringList, QStringList, int, bool)));
    connect(dbManager, SIGNAL(getReceiptData(QStringList,bool)), this,
        SLOT(setCurReceipt(QStringList,bool)));

    curClient = 0;
    curBook = 0;
    trans = 0;
//    currentshiftid = 1; // should change this. Set to 1 for testing;

    MainWindow::setupReportsScreen();

//    //display logged in user and current shift in status bar
//    QLabel *lbl_curUser = new QLabel("Logged in as: " + userLoggedIn + "  ");
//    QLabel *lbl_curShift = new QLabel("Shift Number: ");
//    statusBar()->addPermanentWidget(lbl_curUser);
//    statusBar()->addPermanentWidget(lbl_curShift);
//    lbl_curShift->setText("fda");

    dialog = new QProgressDialog();
    dialog->close();

    // Connect signals and slots for futureWatcher.
    connect(&futureWatcher, SIGNAL(finished()), dialog, SLOT(reset()));
    connect(dialog, SIGNAL(canceled()), &futureWatcher, SLOT(cancel()));
    connect(&futureWatcher, SIGNAL(progressRangeChanged(int,int)), dialog, SLOT(setRange(int,int)));
    connect(&futureWatcher, SIGNAL(progressValueChanged(int)), dialog, SLOT(setValue(int)));

    this->showMaximized();
}

MainWindow::~MainWindow()
{
    delete ui;
}

/*==============================================================================
DETECT WIDGET CHANGING SIGNAL
==============================================================================*/
//initialize widget when getinto that widget
void MainWindow::initCurrentWidget(int idx){
    switch(idx){
        case MAINMENU:  //WIDGET 0
            curClientID = "";
            curClientName = "";
            if(cNew){
                delete(curClient);
                cNew = false;
            }
            if(bNew){
                delete(curBook);
                bNew = false;
            }
            if(tNew){
                delete(trans);
                tNew = false;
            }
            registerType = NOREGISTER;
            ui->actionExport_to_PDF->setEnabled(false);
            ui->actionReceipt->setEnabled(false);
            transType = "";
            isAddressSet();

            //clear booking confirmation fields
            ui->confirmEnd->setText("TextLabel");
            ui->confirmLength->setText("TextLabel");
            ui->confirmMonthly->setText("TextLabel");
            ui->confirmProgram->setText("TextLabel");
            ui->confirmRoom->setText("TextLabel");
            ui->confirmStart->setText("TextLabel");
            ui->confirmCost->setText("TextLabel");
            ui->confirmPaid->setText("TextLabel");
            ui->confirmTotalPaid->setText("TextLabel");
            isRefund = false;
            break;
        case CLIENTLOOKUP:  //WIDGET 1

//            curClientName="";
            qDebug()<<"curclient id in clientLookup before init " << curClientID;
            initClientLookupInfo();
            if(caseWorkerUpdated){
                getCaseWorkerList();
                defaultRegisterOptions();
            }
            ui->tabWidget_cl_info->setCurrentIndex(0);
            if(registerType == EDITCLIENT || registerType == FINDSAMECLIENT){
                on_tableWidget_search_client_itemClicked();
                getClientInfo();
            }
            qDebug()<<"curclient id in CLIENT LOOKUP PAGE: " << curClientID;
            registerType = NOREGISTER;
            set_curClient_name();
            ui->checkBox_search_anonymous->setChecked(false);
            ui->pushButton_search_client->setEnabled(true);
            //initimageview
            ui->actionExport_to_PDF->setEnabled(false);
            receiptid = "";
            ui->le_caseDir->clear();
            break;
        case BOOKINGLOOKUP: //WIDGET 2
            qDebug()<<"###BOOKING LOOKUP Client INFO###";
            ui->bookCostLabel->setText("0");
            if(curClient != NULL){
                qDebug()<<"ID: " << curClientID << curClient->clientId;
                qDebug()<<"NAME: " << curClient->fullName;
                qDebug()<<"Balance: " << curClient->balance;
            }
            ui->startDateEdit->setDate(QDate::currentDate());
            ui->endDateEdit->setDate(QDate::currentDate().addDays(1));
            getProgramCodes();
            bookingSetup();
            clearTable(ui->bookingTable);
            editOverLap = false;
            ui->actionReceipt->setEnabled(false);
            receiptid = "";
            break;
        case BOOKINGPAGE: //WIDGET 3
            //initcode

            break;
        case PAYMENTPAGE: //WIDGET 4
            popManagePayment();

            ui->editRemoveCheque->setHidden(true);
            ui->cbox_payDateRange->setCurrentIndex(1);
            MainWindow::on_cbox_payDateRange_activated(1);

            break;
        case ADMINPAGE: //WIDGET 5
            //initcode
            break;
        case EDITUSERS: //WIDGET 6
            //initcode
            break;
        case EDITPROGRAM: //WIDGET 7
            //initcode
            break;
        case CASEFILE: //WIDGET 8
            ui->chk_filter->setVisible(false); // broken
            ui->tabw_casefiles->setCurrentIndex(PERSIONACASEPLAN);
            ui->tableWidget_casefile_booking->verticalHeader()->show();
            ui->tableWidget_casefile_transaction->verticalHeader()->show();
            qDebug()<<"###CASEFILE Client INFO###";
            if(curClient != NULL){
                qDebug()<<"ID: " << curClientID << curClient->clientId;
                qDebug()<<"NAME: " << curClient->fullName;
                qDebug()<<"Balance: " << curClient->balance;
            }
            newTrans = true;
            newHistory = true;
            newReceipt = true;
            initCasefileTransactionTable();
            initPcp();
            ui->actionExport_to_PDF->setEnabled(true);
            ui->actionReceipt->setEnabled(false);
            break;
        case EDITBOOKING: //WIDGET 9
            ui->editLookupTable->clear();
            ui->editLookupTable->setRowCount(0);
            //initcode
            ui->actionExport_to_PDF->setEnabled(true);
            break;
        case CLIENTREGISTER:    //WIDGET 10
            ui->actionReceipt->setEnabled(false);
            ignore_duplicate = false;
            if((registerType == EDITCLIENT || registerType == DELETECLIENT)
                    && (currentrole == CASEWORKER || currentrole == ADMIN)){
                ui->button_delete_client->show();
                ui->button_delete_client->setEnabled(true);
            }
            else
                ui->button_delete_client->hide();

            clear_client_register_form();
            defaultRegisterOptions();           //combobox item add
            if(curClientID != NULL || curClientID != "")
                read_curClient_Information(curClientID);
            break;
        case REPORTS:    //WIDGET 11
            ui->swdg_reports->setCurrentIndex(DAILYREPORT);
            ui->dailyReport_tabWidget->setCurrentIndex(DEFAULTTAB);
            ui->shiftReport_tabWidget->setCurrentIndex(DEFAULTTAB);
            MainWindow::updateDailyReportTables(QDate::currentDate());
            MainWindow::getDailyReportStats(QDate::currentDate());
            MainWindow::updateShiftReportTables(QDate::currentDate(), currentshiftid);
            MainWindow::getShiftReportStats(QDate::currentDate(), currentshiftid);
            MainWindow::getCashFloat(QDate::currentDate(), currentshiftid);
            MainWindow::getMonthlyReport(QDate::currentDate().month(), QDate::currentDate().year());
            MainWindow::updateRestrictionTables();
            MainWindow::setReportsDateEditMax();
            ui->actionExport_to_PDF->setEnabled(true);
            break;
        case EDITSHIFT:  //WIDGET15
            showAllShiftEdit(true);
            on_checkBox_shift_auto_endtime_clicked(true);
             ReadCurrentShift();
             ui->checkBox_shift_auto_endtime->setChecked(true);
             ui->shift_dayOpt->setCurrentIndex(0);
             ui->shift_num->setCurrentIndex(0);
             ui->pushButton_shift_save->setEnabled(false);
            break;
        case EDITADDRESS:
            break;
        case ROOMHISTORY:
            break;

        default:
            qDebug()<<"NO information about stackWidget idx : "<<idx;

    }
}

void MainWindow::on_bookButton_clicked()
{
    workFlow = BOOKINGPAGE;
    ui->stackedWidget->setCurrentIndex(CLIENTLOOKUP);
    addHistory(MAINMENU);
    qDebug() << "pushed page " << MAINMENU;
    /*ui->startDateEdit->setDate(QDate::currentDate());
    ui->endDateEdit->setDate(QDate::currentDate().addDays(1));
    if(firstTime){
        firstTime = false;
        getProgramCodes();
        bookingSetup();

    }
    ui->makeBookingButton->hide();
    ui->monthCheck->setChecked(false);
    */
}

void MainWindow::on_clientButton_clicked()
{
     workFlow = CLIENTLOOKUP;
     ui->stackedWidget->setCurrentIndex(CLIENTLOOKUP);
     addHistory(MAINMENU);
     qDebug() << "pushed page " << MAINMENU;
}

void MainWindow::on_paymentButton_clicked()
{
     workFlow = PAYMENTPAGE;
     ui->stackedWidget->setCurrentIndex(PAYMENTPAGE);
     addHistory(MAINMENU);
     qDebug() << "pushed page " << MAINMENU;
}

void MainWindow::on_editbookButton_clicked()
{
    const bool blocked = ui->cbo_reg_bldg->blockSignals(true);
    ui->stackedWidget->setCurrentIndex(EDITBOOKING);
    addHistory(MAINMENU);
    ui->lbl_regDateVal->setText(QDate::currentDate().toString(Qt::ISODate));
    ui->de_regDate->setDate(QDate::currentDate());

    on_btn_regGo_clicked();

    qDebug() << "pushed page " << MAINMENU;

    checkRegRadioSelection();
    ui->cbo_reg_bldg->clear();
    qDebug() << "\n\nbuilding combo data cleared";
    QSqlQuery results = dbManager->getBuildings();
    populateCombo(ui->cbo_reg_bldg, results);
    on_cbo_reg_bldg_currentTextChanged("");
    qDebug() << "building combobox populated";
    ui->cbo_reg_bldg->blockSignals(blocked);

    ui->de_regDate->setMaximumDate(QDate::currentDate());
    ui->de_regDate->setMinimumDate(QDate(16,6,20));

}
void MainWindow::checkRegRadioSelection()
{
    qDebug() << "checking radio selection";

    ui->cbo_reg_start->clear();
    ui->cbo_reg_start->addItem("All");
    ui->cbo_reg_end->clear();
    ui->cbo_reg_end->addItem("All");
    ui->cbo_reg_floor->setCurrentIndex(0);

    if (ui->rdo_reg_room->isChecked()) {
        ui->cbo_reg_room->setEnabled(false);
        ui->lbl_reg_start->setText("Room Start");
        ui->lbl_reg_end->setText("Room End");
    } else if (ui->rdo_reg_space->isChecked()) {
        ui->cbo_reg_room->setEnabled(true);
        ui->lbl_reg_start->setText("Space Start");
        ui->lbl_reg_end->setText("Space End");
    }

    on_cbo_reg_floor_currentTextChanged("");

}

void MainWindow::populateCombo(QComboBox *emptyCombo, QSqlQuery results) {
    emptyCombo->setEnabled(true);
    emptyCombo->addItem("All");
    qDebug() << "item all added";
    if (results.next()) {
        ui->roomHistory_search_btn->setEnabled(true);
        ui->btn_reg_searchRS->setEnabled(true);
        emptyCombo->addItem(results.value(0).toString());
        while (results.next()){
            emptyCombo->addItem(results.value(0).toString());
        }
    } else {
        emptyCombo->clear();
        emptyCombo->addItem("Invalid");
        emptyCombo->setEnabled(false);
        ui->roomHistory_search_btn->setEnabled(false);
        ui->btn_reg_searchRS->setEnabled(false);
    }
}

void MainWindow::on_rdo_reg_space_toggled(bool checked)
{
    Q_UNUSED(checked);
    MainWindow::checkRegRadioSelection();
}

void MainWindow::on_cbo_reg_bldg_currentTextChanged(const QString &arg1)
{
    qDebug() << "building text changed";
    const bool blocked = ui->cbo_reg_floor->blockSignals(true);
    Q_UNUSED(arg1);
    QSqlQuery results = dbManager->getFloors(ui->cbo_reg_bldg->currentText());
    ui->cbo_reg_floor->clear();
    qDebug() << "floor combo data cleared";
    populateCombo(ui->cbo_reg_floor, results);
    qDebug() << "floor combo populated";
    ui->cbo_reg_floor->blockSignals(blocked);
    on_cbo_reg_floor_currentTextChanged("");
}

void MainWindow::on_cbo_reg_floor_currentTextChanged(const QString &arg1)
{
    qDebug() << "floor text changed";
    const bool blocked = ui->cbo_reg_room->blockSignals(true);
    Q_UNUSED(arg1);
    QSqlQuery results = dbManager->getRooms(ui->cbo_reg_bldg->currentText(), ui->cbo_reg_floor->currentText());
    if (ui->rdo_reg_room->isChecked()) {
        ui->cbo_reg_start->clear();
        ui->cbo_reg_end->clear();
        qDebug() << "start and end cleared";
        populateCombo(ui->cbo_reg_start, results);
        results.seek(-1);
        populateCombo(ui->cbo_reg_end, results);
        qDebug() << "room start end populated";
    } else {
        ui->cbo_reg_room->clear();
        qDebug() << "room combo data cleared";
        populateCombo(ui->cbo_reg_room, results);
        qDebug() << "room combo populated";
        on_cbo_reg_room_currentTextChanged("");
    }
    ui->cbo_reg_room->blockSignals(blocked);
}

void MainWindow::on_cbo_reg_room_currentTextChanged(const QString &arg1)
{
    qDebug() << "room text changed";
    Q_UNUSED(arg1);
    QSqlQuery results = dbManager->getSpaces(ui->cbo_reg_bldg->currentText(), ui->cbo_reg_floor->currentText(), ui->cbo_reg_room->currentText());
    ui->cbo_reg_start->clear();
    ui->cbo_reg_end->clear();
    qDebug() << "start and end cleared";
    populateCombo(ui->cbo_reg_start, results);
    results.seek(-1);
    populateCombo(ui->cbo_reg_end, results);
    qDebug() << "space start end populated";
}

//void MainWindow::populateRegStartEnd(QString type){
//    qDebug() << "populate combo type: " << type;
//}

void MainWindow::on_roomHistoryButton_clicked()
{
    //ui->stackedWidget->setCurrentIndex(ROOMHISTORY);
    
    //int buildingNo = -1;
    //int floorNo = -1;
    //int roomNo = -1;
    //int spaceNo = -1;

    const bool blocked = ui->building_cbox->blockSignals(true);
    ui->stackedWidget->setCurrentIndex(ROOMHISTORY);
    addHistory(MAINMENU);

    /*
    QSqlQuery query;
    if (dbManager->getRoomHistory(&query, buildingNo, floorNo, roomNo, spaceNo))
    {
        QStringList header;
        QStringList cols;
        header << "Client" << "Space Code" << "Program" << "Date" << "Start Date" 
               << "End Date" << "Action" << "Employee" << "Shift #" << "Time";
        cols << "ClientName" << "SpaceCode" << "ProgramCode" << "Date" << "StartDate" 
             << "EndDate" << "Action" << "EmpName" << "ShiftNo" << "Time";
        populateATable(ui->roomHistory_tableWidget, header, cols, query, false);
        resizeTableView(ui->roomHistory_tableWidget);  
    }*/

    //on_editSearch_clicked();

    qDebug() << "pushed page " << MAINMENU;

    //checkRegRadioSelection();
    ui->building_cbox->clear();
    qDebug() << "\n\nbuilding combo data cleared";
    QSqlQuery results = dbManager->getBuildings();
    populateCombo(ui->building_cbox, results);
    on_building_cbox_currentTextChanged("");
    qDebug() << "building combobox populated";
    ui->building_cbox->blockSignals(blocked);
    MainWindow::on_roomHistory_search_btn_clicked();
}

void MainWindow::on_caseButton_clicked()
{
    workFlow = CASEFILE;
    ui->stackedWidget->setCurrentIndex(CLIENTLOOKUP);
    addHistory(MAINMENU);
    qDebug() << "pushed page " << MAINMENU;

}

void MainWindow::on_adminButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(ADMINPAGE);
    addHistory(MAINMENU);
    qDebug() << "pushed page " << MAINMENU;

}

/*==============================================================================
DEV TESTING BUTTONS (START)
==============================================================================*/
void MainWindow::on_actionDB_Connection_triggered()
{
    QString firstName = ui->lineEdit_cl_fName->text();
    QString middleName = ui->lineEdit_cl_mName->text();
    QString lastName = ui->lineEdit_cl_lName->text();

    if (!firstName.isEmpty())
        firstName[0] = firstName[0].toUpper();
    if (!middleName.isEmpty())
        middleName[0] = middleName[0].toUpper();
    if (!lastName.isEmpty())
        lastName[0] = lastName[0].toUpper();

    qDebug() << firstName << middleName << lastName;
}

void MainWindow::on_actionTest_Query_triggered()
{
    dbManager->printAll(dbManager->selectAll("Client"));
}

void MainWindow::on_actionFile_Upload_triggered()
{
    QString strFilePath = MainWindow::browse();
    if (!strFilePath.isEmpty())
    {
        QtConcurrent::run(dbManager, &DatabaseManager::uploadThread, strFilePath);
    }
    else
    {
        qDebug() << "Empty file path";
    }
}

void MainWindow::on_actionDownload_Latest_Upload_triggered()
{
    QtConcurrent::run(dbManager, &DatabaseManager::downloadThread);
}

void MainWindow::on_actionPrint_Db_Connections_triggered()
{
    dbManager->printDbConnections();
}

void MainWindow::on_actionUpload_Display_Picture_triggered()
{
    QString strFilePath = MainWindow::browse();
    if (!strFilePath.isEmpty())
    {
        QtConcurrent::run(dbManager, &DatabaseManager::uploadProfilePicThread, strFilePath);
    }
    else
    {
        qDebug() << "Empty file path";
    }
}

void MainWindow::on_actionDownload_Profile_Picture_triggered()
{
    QImage* img = new QImage();
    img->scaledToWidth(300);
    dbManager->downloadProfilePic(img);

    MainWindow::addPic(*img);
}
/*==============================================================================
DEV TESTING BUTTONS (END)
==============================================================================*/
/*==============================================================================
DEV TESTING AUXILIARY FUNCTIONS (START)
==============================================================================*/
QString MainWindow::browse()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::DirectoryOnly);
    QString strFilePath = dialog.getOpenFileName(this, tr("SelectFile"), "", tr("All Files (*.*)"));

    return strFilePath;
}
/*==============================================================================
DEV TESTING AUXILIARY FUNCTIONS (END)
==============================================================================*/

//COLIN STUFF /////////////////////////////////////////////////////////////////

void MainWindow::on_lunchCheck_clicked()
{
   QDate testDate = QDate::currentDate();
   testDate = testDate.addDays(32);
//   QDate otherDate = testDate.addDays(35);
  //curClient = new Client();
   //curClient->clientId = "1";
    QString tmpStyleSheet = MainWindow::styleSheet();
    MainWindow::setStyleSheet("");

   MyCalendar* mc = new MyCalendar(this, curBook->startDate,curBook->endDate, curClient,1, curBook->room);
   mc->exec();
   delete(mc);

   MainWindow::setStyleSheet(tmpStyleSheet);
}

void MainWindow::on_paymentButton_2_clicked()
{
    trans = new transaction();
    tNew = true;
    trans->paidToday = 0;
    double owed;
    //owed = curBook->cost;
    owed = ui->costInput->text().toDouble();
    QString note = "Booking: " + curBook->stringStart + " to " + curBook->stringEnd + " Cost: " + QString::number(curBook->cost, 'f', 2);
    payment * pay = new payment(this, trans, curClient->balance, owed , curClient, note, true, usernameLoggedIn, QString::number(currentshiftid));
    pay->exec();
    ui->stayLabel->setText(QString::number(curClient->balance, 'f', 2));
    qDebug() << "Done";
    delete(pay);
    double totalPaid = ui->bookAmtPaid->text().toDouble();
    totalPaid += trans->paidToday;
    curBook->paidTotal = totalPaid;
    if(totalPaid < 0){
        ui->bookLabelPaid->setText("Total Refunded:");
        ui->bookAmtPaid->setText(QString::number(totalPaid * -1, 'f', 2));

    }
    else{
        ui->bookLabelPaid->setText("Total Paid:");
        ui->bookAmtPaid->setText(QString::number(totalPaid, 'f', 2));
    }
    transType = trans->type;

    if (ui->bookAmtPaid->text() != "0.00"){
        saveReceipt(false, ui->bookAmtPaid->text(), false);
    }

}
void MainWindow::on_startDateEdit_dateChanged()
{

    if(ui->startDateEdit->date() < QDate::currentDate()){
        ui->startDateEdit->setDate(QDate::currentDate());
    }
    if(ui->startDateEdit->date() > ui->endDateEdit->date()){
        QDate newD = ui->startDateEdit->date().addDays(1);
        ui->endDateEdit->setDate(newD);
    }
    clearTable(ui->bookingTable);
    ui->makeBookingButton->hide();
    ui->bookCostLabel->setText("0");
}

void MainWindow::on_wakeupCheck_clicked()
{
    QString tmpStyleSheet = MainWindow::styleSheet();
    MainWindow::setStyleSheet("");
    
    MyCalendar* mc = new MyCalendar(this, curBook->startDate,curBook->endDate, curClient,2, curBook->room);
    mc->exec();
    delete(mc);

    MainWindow::setStyleSheet(tmpStyleSheet);
}

void MainWindow::on_endDateEdit_dateChanged()
{
    if(editOverLap){
        editOverLap = false;
    }
    else{
        editOverLap = false;
        //ui->monthCheck->setChecked(false);
    }
    if(ui->endDateEdit->date() < ui->startDateEdit->date()){
        QDate newDate = ui->startDateEdit->date().addDays(1);
        ui->endDateEdit->setDate(newDate);

    }
    clearTable(ui->bookingTable);
    std::pair<int,int> p = monthDay(ui->startDateEdit->date(), ui->endDateEdit->date());
    ui->bookMonthLabel->setText(QString::number(p.first));
    ui->bookDayLabel->setText(QString::number(p.second));
    ui->makeBookingButton->hide();
    ui->bookCostLabel->setText("0");
}

void MainWindow::on_monthCheck_clicked(bool checked)
{
    clearTable(ui->bookingTable);
    ui->makeBookingButton->hide();
    ui->bookCostLabel->setText("0");
    if(checked)
    {
        editOverLap = true;
        QDate month = ui->startDateEdit->date();
        //month = month.addMonths(1);
        int days = month.daysInMonth();
        days = days - month.day();
        month = month.addDays(days + 1);
        ui->endDateEdit->setDate(month);
        //ui->monthCheck->setChecked(true);
    }
    else{
       // ui->monthCheck->setChecked(false);
        editOverLap = true;
    }
}
void MainWindow::bookingSetup(){

    ui->bookingTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->bookingTable->verticalHeader()->hide();
   // ui->bookingTable->horizontalHeader()->setStretchLastSection(true);
    ui->bookingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->bookingTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->bookingTable->setHorizontalHeaderLabels(QStringList() << "Room #" << "Location" << "Program" << "Description" << "Daily Cost" << "Monthly Cost");

}
void MainWindow::clearTable(QTableWidget * table){
    table->clearContents();
    table->setRowCount(0);
}

void MainWindow::on_editButton_clicked()
{
    ui->actionExport_to_PDF->setEnabled(false);
    addHistory(EDITBOOKING);
    setup = true;
    int row = ui->editLookupTable->selectionModel()->currentIndex().row();
    if(row == - 1){
        return;
    }
    ui->editDate->setEnabled(true);
    ui->editRoom->setEnabled(true);
    curBook = new Booking();
    bNew = true;

    popBookFromRow();
    popClientFromId(curBook->clientId);
    ui->editUpdate->setEnabled(false);
    popEditPage();
    setBookSummary();
    if(!getRoomCosts())
        return;
    setEditDayInfo(curBook->endDate);
    ui->stackedWidget->setCurrentIndex(EDITPAGE);
    ui->editUpdate->setEnabled(false);
    setup = false;
}
bool MainWindow::getRoomCosts(){
    dCost = 0;
    mCost = 0;
    editExpected = 0;
    QSqlQuery result = dbManager->getRoomCosts(curBook->roomId);
    if(!result.next())
        return false;
    dCost = result.value("cost").toString().toDouble();
    mCost = result.value("Monthly").toString().toDouble();
    editExpected = realCost(curBook->startDate, curBook->endDate, dCost, mCost);
    return true;
}

void MainWindow::popClientFromId(QString id){
    QSqlQuery result;
    curClient = new Client();
    cNew = true;

    result = dbManager->pullClient(id);
    result.seek(0, false);
    curClient->clientId = id;
    curClient->fName = result.record().value("FirstName").toString();
    curClient->mName = result.record().value("MiddleName").toString();
    curClient->lName = result.record().value("LastName").toString();
    curClient->fullName = curClient->fName + " " +  curClient->mName + " "
            + curClient->lName;
    QString balanceString = result.record().value("Balance").toString();
    balanceString.replace("$", "");
    curClient->balance =  balanceString.toDouble();
    // curClient->balance = result.record().value("Balance").toString().toDouble();


}

void MainWindow::popEditPage(){

    QSqlQuery result;
    result = dbManager->getPrograms();
    QString curProgram;
    QStringList programs;
    QString compProgram;
    int index = 0;
    int x = 0;
    ui->editOC->setText(QString::number(curBook->cost,'f',2));
    curProgram = curBook->program;
    while(result.next()){
        QString compProgram = result.value("ProgramCode").toString();
        if(curProgram == compProgram)
            index =x;
        programs << compProgram;
        x++;
    }
    ui->editProgramDrop->clear();
    ui->editProgramDrop->addItems(programs);
    ui->editProgramDrop->setCurrentIndex(index);
    ui->editRoomLabel->setText(curBook->room);
    ui->editDate->setDate(curBook->endDate);
    ui->editCost->setText(QString::number(curBook->cost));
    ui->editRoom->setEnabled(true);

}

void MainWindow::popBookFromRow(){
    int row = ui->editLookupTable->selectionModel()->currentIndex().row();
    if(row == - 1){
        return;
    }
    QString cost = ui->editLookupTable->item(row,5)->text();
    if (cost.at(0) == '$') {
        curBook->cost = cost.right(cost.length()-1).toDouble();
    } else {
        curBook->cost = cost.toDouble();
    }
    curBook->stringStart = ui->editLookupTable->item(row, 2)->text();
    curBook->startDate = QDate::fromString(curBook->stringStart, "yyyy-MM-dd");
    curBook->stringEnd = ui->editLookupTable->item(row, 3)->text();
    curBook->endDate = QDate::fromString(curBook->stringEnd, "yyyy-MM-dd");
    curBook->stayLength = curBook->endDate.toJulianDay() - curBook->startDate.toJulianDay();
    if(ui->editLookupTable->item(row,6)->text() == "YES"){
      curBook->monthly = true;
         }
     else{
         curBook->monthly = false;
     }
    curBook->clientId = ui->editLookupTable->item(row, 8)->text();

    curBook->program = ui->editLookupTable->item(row,4)->text();
    curBook->room = ui->editLookupTable->item(row,1)->text();
    curBook->bookID = ui->editLookupTable->item(row, 7)->text();
    curBook->roomId = ui->editLookupTable->item(row, 9)->text();

}
void MainWindow::popManagePayment(){
    QStringList dropItems;
    ui->cbox_payDateRange->clear();
    ui->mpTable->clear();
    ui->mpTable->setRowCount(0);
    ui->btn_payDelete->setText("Delete");

    dropItems << "" << "Today" << "Last 3 Days" << "This Month"
              <<  QDate::longMonthName(QDate::currentDate().month() - 1)
              << QDate::longMonthName(QDate::currentDate().month() - 2)
              << "ALL";
    ui->cbox_payDateRange->addItems(dropItems);
}

void MainWindow::on_cbox_payDateRange_activated(int index)
{
    ui->editRemoveCheque->setHidden(true);

    QString startDate;
    QDate endDate = QDate::currentDate();
    QDate hold = QDate::currentDate();
    ui->btn_payDelete->setText("Delete");
    int days, move;
    switch(index){
    case 0:
        return;
        break;
    case 1:
        startDate = QDate::currentDate().toString(Qt::ISODate);
        break;
    case 2:
        hold = hold.addDays(-3);
        startDate = hold.toString(Qt::ISODate);
        break;
    case 3:
        days = hold.daysInMonth();
        days = days - hold.day();
        endDate = hold.addDays(days);
        move = hold.day() -1;
        hold = hold.addDays(move * -1);
        break;
    case 4:
        hold = hold.addMonths(-1);
        days = hold.daysInMonth();

        move = hold.day() -1;
        days = days - hold.day();
        endDate = hold.addDays(days);
        hold = hold.addDays(move * -1);
        break;
    case 5:
        hold = hold.addMonths(-2);
        days = hold.daysInMonth();

        move = hold.day() -1;
        days = days - hold.day();
        endDate = hold.addDays(days);
        hold = hold.addDays(move * -1);
        break;
    case 6:
        hold = QDate::fromString("1970-01-01", "yyyy-MM-dd");
        endDate = QDate::fromString("2222-01-01", "yyyy-MM-dd");
        break;

    }
    QStringList heads;
    QStringList cols;
    QSqlQuery tempSql = dbManager->getTransactions(hold, endDate);
    heads << "Date"  <<"First Name" << "Last Name" << "Amount" << "Type" << "Method" << "Notes"  << "" << "" << "Employee Name";
    cols << "Date" <<"FirstName"<< "LastName"  << "Amount" << "TransType" << "Type" << "Notes" << "TransacId" << "ClientId" << "EmpName";
    populateATable(ui->mpTable, heads, cols, tempSql, false);
    addCurrencyNoSignToTableWidget(ui->mpTable, 3);
    ui->mpTable->setColumnHidden(4, false);
    ui->mpTable->setColumnHidden(5, false);
    ui->mpTable->setColumnHidden(6, false);
    ui->mpTable->setColumnHidden(7, true);
    ui->mpTable->setColumnHidden(8, true);
    resizeTableView(ui->mpTable);

}
void MainWindow::doValidate(){
    QSqlQuery result;
    result = dbManager->getAllClients();
    QTableWidget * table = ui->valTable;
    QStringList headers;
    headers << "ID" << "Name" << "Balance" << "Expected";
    table->clear();
    table->setRowCount(0);

    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->hide();
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    int colCount = headers.size();
    table->setColumnCount(colCount);
    if(headers.length() != 0){
        table->setHorizontalHeaderLabels(headers);
    }
    double expected, real;
    int x = 0;
    while(result.next()){
        QString clientId = result.value("ClientId").toString();
        expected = dbManager->validateMoney(clientId);
        real = result.value("Balance").toString().toDouble();
        double tmp = QString::number(expected, 'f', 2).toDouble();
        expected = tmp;
        if(expected == real){
            continue;
        }
        qDebug() << "\n\n Clientid: " << clientId << " expected: " << expected << " real: " << real;
        table->insertRow(x);
        qDebug() << "Adding Client" << clientId;
        QString fullName = result.value("FirstName").toString() + result.value("LastName").toString();
        table->setItem(x,0, new QTableWidgetItem(result.value("ClientId").toString()));
        table->setItem(x,1, new QTableWidgetItem(fullName));
        table->setItem(x,2, new QTableWidgetItem(QString::number(real, 'f',2)));
        table->setItem(x,3, new QTableWidgetItem(QString::number(expected, 'f', 2)));
        x++;
    }

    MainWindow::resizeTableView(table);
}

void MainWindow::on_btn_payListAllUsers_clicked()
{
    ui->editRemoveCheque->setHidden(true);

    ui->btn_payDelete->setText("Edit Payment"); //Shaily Change**********

    QStringList cols;
    QStringList heads;
    QSqlQuery tempSql = dbManager->getOwingClients();
    heads << "First" << "Last" << "DOB" << "Balance" << "";
    cols << "FirstName" << "LastName" << "Dob" << "Balance" << "ClientId";
    ui->mpTable->setColumnHidden(4, true);
    populateATable(ui->mpTable, heads, cols, tempSql, false);
    addCurrencyNoSignToTableWidget(ui->mpTable, 3);
    resizeTableView(ui->mpTable);
}

void MainWindow::on_editSearch_clicked()
{
    QStringList nameList = ui->editClient->text().split(" ");
//    QString name = ui->editClient->text();


    for(int i = 0; i < ui->editLookupTable->rowCount(); ++i) {
        bool match = true;
        QTableWidgetItem *item = ui->editLookupTable->item( i, 0 );

        for (int j = 0; j < nameList.size(); ++j) {
            QRegularExpression pattern(nameList[j], QRegularExpression::CaseInsensitiveOption);
            if (!pattern.match(item->text()).hasMatch()){
                match = false;
                break;
            }
            qDebug() << "match at row " << i << ": " << match;
        }

        //hide rows that don't match pattern
        ui->editLookupTable->setRowHidden( i, !match );
    }

}
void MainWindow::on_bookingSearchButton_clicked()
{
    if(!book.checkValidDate(ui->startDateEdit->date(), ui->endDateEdit->date())){
        //Pop up error or something
        return;
    }
    QString program = ui->programDropdown->currentText();

    QSqlQuery result = dbManager->getCurrentBooking(ui->startDateEdit->date(), ui->endDateEdit->date(), program);
    ui->bookCostLabel->setText("0");
   /* ui->bookingTable->setRowCount(0);
    ui->bookingTable->clear();
    ui->bookingTable->setHorizontalHeaderLabels(QStringList() << "Room #" << "Location" << "Program" << "Description" << "Cost" << "Monthly");
    int numCols = result.record().count();

    int x = 0;
    while (result.next()) {
        ui->bookingTable->insertRow(x);
        for (int i = 0; i < numCols; ++i)
        {
            if(i == 4){
                ui->bookingTable->setItem(x,i, new QTableWidgetItem(QString::number(result.value(i).toString().toDouble(), 'f', 2)));
                continue;
            }
            ui->bookingTable->setItem(x,i, new QTableWidgetItem(result.value(i).toString()));


        }

        x++;

    }
*/
    QStringList headers, cols;
    headers << "Space #" << "Program" << " Type" << "Daily Cost" << "Monthly Cost" << "";
    cols << "SpaceCode" << "ProgramCodes" << "type" << "cost" << "Monthly" << "SpaceId";
    populateATable(ui->bookingTable, headers, cols, result,false);
    ui->bookingTable->setColumnHidden(5, true);
    ui->makeBookingButton->show();
    MainWindow::resizeTableView(ui->bookingTable);
    MainWindow::addCurrencyNoSignToTableWidget(ui->bookingTable, 3);
    MainWindow::addCurrencyNoSignToTableWidget(ui->bookingTable, 4);
}
//PARAMS - The table, list of headers, list of table column names, the sqlquery result, STRETCH - stretch mode true/false
void MainWindow::populateATable(QTableWidget * table, QStringList headers, QStringList items, QSqlQuery result, bool stretch){
    table->clear();
    table->setRowCount(0);

    if(headers.length() != items.length())
        return;

    if(stretch)
        table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->hide();
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    int colCount = headers.size();
    table->setColumnCount(colCount);
    if(headers.length() != 0){
        table->setHorizontalHeaderLabels(headers);
    }
    int x = 0;
    while(result.next()){
        table->insertRow(x);
        for(int i = 0; i < colCount; i++){
            table->setItem(x, i, new QTableWidgetItem(result.value(items.at(i)).toString()));


        }


        x++;
    }
}

void MainWindow::setBooking(int row){
    curBook->clientId = curClient->clientId;
    curBook->startDate = ui->startDateEdit->date();
    curBook->endDate = ui->endDateEdit->date();
    curBook->stringStart = ui->startDateEdit->date().toString(Qt::ISODate);
    curBook->stringEnd = ui->endDateEdit->date().toString(Qt::ISODate);
    //curBook->monthly = ui->monthCheck->isChecked();
    curBook->monthly = false;
    curBook->program = ui->programDropdown->currentText();
    curBook->room = ui->bookingTable->item(row,0)->text();
    curBook->stayLength = ui->endDateEdit->date().toJulianDay() - ui->startDateEdit->date().toJulianDay();
    curBook->roomId = ui->bookingTable->item(row, 5)->text();
    double daily, monthly;
    daily = ui->bookingTable->item(row, 3)->text().toDouble();
    monthly = ui->bookingTable->item(row, 4)->text().toDouble();
    double cost;
    cost = realCost(curBook->startDate, curBook->endDate, daily, monthly);
    curBook->cost = cost;

}

void MainWindow::on_makeBookingButton_clicked()
{
    addHistory(BOOKINGLOOKUP);
    int row = ui->bookingTable->selectionModel()->currentIndex().row();
    if(row == - 1){
        return;
    }
    //curClient = new Client();
   //popClientFromId("1");
//    int rowNum = ui->bookingTable->columnCount();
    QStringList data;
    curBook = new Booking;
    bNew = true;
    setBooking(row);
    ui->stackedWidget->setCurrentIndex(BOOKINGPAGE);
    populateBooking();
    ui->makeBookingButton_2->setEnabled(true);
    ui->bookAmtPaid->setText("0.00");
    ui->confirmTotalPaid->setText("0.00");

}
void MainWindow::populateBooking(){
    //room, location, program, description, cost, program, start, end, stayLength
    ui->startLabel->setText(curBook->stringStart);
    ui->endLabel->setText(curBook->stringEnd);
    ui->roomLabel->setText(curBook->room);
    ui->costInput->setText(QString::number(curBook->cost, 'f', 2));
    ui->programLabel->setText(curBook->program);
    ui->lengthOfStayLabel->setText(QString::number(curBook->stayLength));
    // - curBook->cost + curBook->paidTotal, 'f', 2)
    ui->stayLabel->setText(QString::number(curClient->balance, 'f', 2));
    if(curBook->monthly){
        ui->monthLabel->setText("YES");
    }
    else{
        ui->monthLabel->setText("NO");
    }

}

void MainWindow::getProgramCodes(){
    QSqlQuery result = dbManager->getPrograms();
//    int i = 0;
    ui->programDropdown->clear();
    while(result.next()){
        ui->programDropdown->addItem(result.value(0).toString());
    }
}
void MainWindow::setBookSummary(){
    ui->editStartDate->setText(curBook->stringStart);
    ui->editEndDate->setText(curBook->stringEnd);
    QDate end = QDate::fromString(curBook->stringEnd, "yyyy-MM-dd");
    int length = end.toJulianDay() - curBook->startDate.toJulianDay();
    curBook->endDate = end;
    curBook->stayLength = length;
    ui->editCostLabel->setText("$"+QString::number(curBook->cost, 'f', 2));
    curBook->monthly == true ? ui->editMonthly->setText("Yes") : ui->editMonthly->setText("No");
    ui->editProgramLabel->setText(curBook->program);
    ui->editLengthOfStay->setText(QString::number(curBook->stayLength));
    ui->editRoomLabel_2->setText(curBook->room);
    ui->editCost->setText(QString::number(curBook->cost, 'f', 2));


    //ui->editRefundAmt->setText(QString::number(ui->editOC->text().toDouble() - curBook->cost));

}

void MainWindow::on_editUpdate_clicked()
{
    ui->editUpdate->setEnabled(false);
    double updateBalance;
    if(!checkNumber(ui->editCost->text()))
        return;

    ui->editRoom->setEnabled(true);
    ui->editDate->setEnabled(true);
    ui->editLunches->setEnabled(true);
    ui->editWakeup->setEnabled(true);
    if(ui->editRefundLabel->text() == "Refund"){
        curBook->monthly = false;
        updateBalance = curClient->balance + ui->editRefundAmt->text().toDouble();
    }
    else{
        updateBalance = curClient->balance - ui->editRefundAmt->text().toDouble();
    }
    curBook->cost = ui->editCost->text().toDouble();
    if(!dbManager->updateBalance(updateBalance, curClient->clientId)){
        qDebug() << "Error inserting new balance";
        return;
    }
    ui->editOC->setText(QString::number(curBook->cost, 'f', 2));
//    double qt = ui->editRefundAmt->text().toDouble();
    ui->editRefundAmt->setText("0.0");

    curClient->balance = updateBalance;
    curBook->endDate = ui->editDate->date();
    curBook->stringEnd = curBook->endDate.toString(Qt::ISODate);
    curBook->program = ui->editProgramDrop->currentText();
    updateBooking(*curBook);
    setBookSummary();
    dbManager->removeLunchesMulti(curBook->endDate, curClient->clientId);
    dbManager->deleteWakeupsMulti(curBook->endDate, curClient->clientId);
    curBook->stayLength = curBook->endDate.toJulianDay() - curBook->startDate.toJulianDay();
    dbManager->addHistoryFromId(curBook->bookID, userLoggedIn, QString::number(currentshiftid), "EDIT");
}

bool MainWindow::doMessageBox(QString message){
    QString tmpStyleSheet=this->styleSheet();
    this->setStyleSheet("");

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm ", message, QMessageBox::Yes | QMessageBox::No);

    this->setStyleSheet(tmpStyleSheet);

    if(reply == QMessageBox::Yes){
        return true;
    }
    return false;

}

double MainWindow::calcRefund(QDate old, QDate n){
    int updatedDays = n.toJulianDay() - old.toJulianDay();
    double cpd;
    double updatedCost;
    if(updatedDays > 0){
        //DO A POPUP ERROR HERE
        if(curBook->monthly){
            qDebug() << "NOT POSSIBLE -> is a monthly booking";
            ui->editDate->setDate(curBook->endDate);
            updatedDays = 0;
        }
        cpd = curBook->cost / (double)curBook->stayLength;
        //curBook->cost += updatedDays * cpd;
        return updatedDays * cpd;
    }
    else{
        //ERROR POPUP FOR INVALID DATe
        if(n < curBook->startDate){


            ui->editDate->setDate(curBook->startDate);
            updatedDays = curBook->stayLength * -1;

        }
        cpd = curBook->cost / (double)curBook->stayLength;
        updatedCost = cpd * (curBook->stayLength + updatedDays);

        if(curBook->monthly){
            double normalRate = dbManager->getRoomCost(curBook->roomId);
            updatedCost = normalRate * (curBook->stayLength + updatedDays);
            if(updatedCost > curBook->cost)
                return 0;

        }
        return (curBook->cost - updatedCost) * -1;
    }
}

bool MainWindow::checkNumber(QString num){
    int l = num.length();
    if(l > 8)
        return false;
    int period = 0;
    char copy[l];
    strcpy(copy, num.toStdString().c_str());
    for(int i = 0; i < num.length(); i++){
        if(copy[i] == '.'){
            if(period)
                return false;
            period++;
            continue;
        }

        if(!isdigit(copy[i]))
            return false;
    }
    return true;
}
bool MainWindow::updateBooking(Booking b){
    qDebug() << "updateBooking called";
    QString query;
    QString monthly;
    curBook->monthly == true ? monthly = "YES" : monthly = "NO";
    QStringList spaceInfo = dbManager->getSpaceInfoFromId(b.roomId.toInt());
    QString addSpaceInfoString = "";
    if (spaceInfo.isEmpty())
    {
        qDebug() << "Empty spaceInfo list";
    }
    else
    {
        addSpaceInfoString = QString("BuildingNo = '" + spaceInfo.at(0) + "', ")    
            + QString("FloorNo = '" + spaceInfo.at(1) + "', ")
            + QString("RoomNo = '" + spaceInfo.at(2) + "', ")
            + QString("SpaceNo = '" + spaceInfo.at(3) + "', ")
            + QString("SpaceCode = '" + spaceInfo.at(4) + "', ");
    }

    query = "UPDATE BOOKING SET " +
            QString("SpaceID = '") + b.roomId + "', " + //addSpaceInfoString +
            QString("ProgramCode = '") + b.program + "', " +
            QString("Cost = '") + QString::number(b.cost) + + "', " +
            QString("EndDate = '") + b.stringEnd + "', " +
            QString("Monthly = '") + monthly + "'" +
            QString(" WHERE BookingId = '") +
            b.bookID + "'";
    //qDebug() << query;
    return dbManager->updateBooking(query);
}

//-------------------------Shaily's Changes--------------------------
void MainWindow::on_btn_payDelete_clicked()
{
	ui->btn_payDelete->setHidden(false);
	 QMessageBox msgBox;
            
    if(ui->btn_payDelete->text() == "Cash Cheque")
    {
		//if ((currentrole == ADMIN) ||(currentrole != ADMIN)){
		//	ui->btn_payDelete->setHidden(true);
		int index = ui->mpTable->selectionModel()->currentIndex().row();
        if(index == -1)
            return;
        transaction * t = new transaction();
        tNew = true;
        t->chequeNo = "";
        AddMSD * amd = new AddMSD(this,t);
        amd->exec();
        if(t->chequeNo == "NO")
            return;
        updateCheque(index, t->chequeNo);
        delete(amd);
		
		//}
		
      
    }
    else if(ui->btn_payDelete->text() == "Delete"){
        if(currentrole != ADMIN ){
        //get a dealog box to show up
		msgBox.setText("Sorry you don't have permissions to delete a payment, Admin Only Task");
        msgBox.exec();
		ui->btn_payDelete->hide();
		}else{
		
			//ui->btn_payDelete->setHidden(false);
			ui->btn_payDelete->show();
		int index = ui->mpTable->selectionModel()->currentIndex().row();
        if(index == -1)
            return;
		
        getTransactionFromRow(index);
		}
		
    }
    else{
		if(currentrole != ADMIN ){
        //get a dealog box to show up
		msgBox.setText("Sorry you don't have permissions to edit this payment, Admin Only Task");
        msgBox.exec();
		ui->btn_payDelete->hide();
		}
		else{ 
		int index = ui->mpTable->selectionModel()->currentIndex().row();
        if(index == -1)
            return;
        handleNewPayment(index);
		}
        
    }
	ui->btn_payDelete->setHidden(false);

}
//------------------------------------------------------------------------------------

void MainWindow::handleNewPayment(int row){
    curClient = new Client();
    cNew = true;
    trans = new transaction();
    tNew = true;
    curClient->clientId = ui->mpTable->item(row,4)->text();
    QString balanceString = ui->mpTable->item(row, 3)->text();
    balanceString.replace("$", "");
    double balance = balanceString.toDouble();
    // double balance = ui->mpTable->item(row, 3)->text().toDouble();
    curClient->balance = balance;
    QString note = "Paying Outstanding Balance";

    payment * pay = new payment(this, trans, curClient->balance, 0 , curClient, note, true, usernameLoggedIn, QString::number(currentshiftid));
    pay->exec();
//    ui->mpTable->removeRow(row);
    delete(pay);

    //text receipt
    curClientID = curClient->clientId;
    getFullName(curClientID);

    qDebug() << "total paid " << QString::number(trans->paidToday, 'f', 2);
    transType = trans->type;
    qDebug() << "trans type" << transType;
    if (transType != ""){
        saveReceipt(false, QString::number(trans->paidToday, 'f', 2), true);
        receiptid = "";
    }

    on_btn_payListAllUsers_clicked();
}

void MainWindow::updateCheque(int row, QString chequeNo){
    QString transId = ui->mpTable->item(row, 6)->text();
    double retAmt = ui->mpTable->item(row, 3)->text().toDouble();
    QString clientId = ui->mpTable->item(row, 5)->text();
    curClient = new Client();
    cNew = true;
    popClientFromId(clientId);
    double curBal = curClient->balance + retAmt;
    if(dbManager->setPaid(transId, chequeNo )){
        if(!dbManager->updateBalance(curBal, clientId)){
                qDebug() << "BIG ERROR - removed transacton but not update balance";
                return;
        }
    }
    ui->mpTable->removeRow(row);
}

void MainWindow::getTransactionFromRow(int row){
    QString transId = ui->mpTable->item(row, 7)->text();

    QString type = ui->mpTable->item(row, 4)->text();
    double retAmt = ui->mpTable->item(row, 3)->text().toDouble();
    QString clientId = ui->mpTable->item(row, 8)->text();
    curClient = new Client();
    cNew = true;
    popClientFromId(clientId);
    double curBal = curClient->balance;

    if(type == "Payment"){
        curBal -= retAmt;
    }
    else if(type == "Refund"){
        curBal += retAmt;
    }
    else{
        //error - not a payment or refund
        return;
    }
    dbManager->updateBalance(curBal, clientId);
    dbManager->removeTransaction(transId);
    ui->mpTable->removeRow(row);
}

void MainWindow::on_btn_payOutstanding_clicked()
{
    ui->btn_payDelete->setText("Cash Cheque");
    ui->editRemoveCheque->setHidden(false);
    QSqlQuery result;
    result = dbManager->getOutstanding();
    QStringList headers;
    QStringList cols;
    headers << "Date" << "First" << "Last" << "Amount" << "Notes" << "" << "";
    cols << "Date" << "FirstName" << "LastName" << "Amount" << "Notes" << "ClientId" << "TransacId";
    populateATable(ui->mpTable, headers, cols, result, false);
    addCurrencyNoSignToTableWidget(ui->mpTable, 3);
    ui->mpTable->setColumnHidden(4, false);
    ui->mpTable->setColumnHidden(6, true);
    ui->mpTable->setColumnHidden(5, true);
    resizeTableView(ui->mpTable);
}


void MainWindow::on_editDate_dateChanged(const QDate &date)
{
    if(!setup){
        ui->editLunches->setEnabled(false);
        ui->editWakeup->setEnabled(false);
        ui->editUpdate->setEnabled(true);
        statusBar()->showMessage(tr("Room change, edit lunches and edit wakeups disabled because dates changed. Reload this page to edit these."), 10000);
    }

    if(date < QDate::currentDate()){
        ui->editDate->setDate(QDate::currentDate());

    }
    ui->editRoom->setEnabled(false);

    QDate nextStart = date;
    QDate comp;
    if(date > curBook->endDate){
        QSqlQuery result;
        result = dbManager->getNextBooking(curBook->endDate, curBook->roomId);
        int x = 0 ;
        if(!result.next()){
            nextStart = date;
        }

        else{
            nextStart = QDate::fromString(result.value("StartDate").toString(), "yyyy-MM-dd");

        }

        ui->editDate->setDate(nextStart);
    }

    editExpected = realCost(curBook->startDate, curBook->endDate, dCost, mCost);
    setEditDayInfo(date);
    if(curBook->cost != editExpected){

    }

    qDebug() << "Edit date called";
    double refund = 0;
    double newCost;
    QString cost = ui->editCost->text();
    if(!checkNumber(cost)){
        qDebug() << "NON NUMBER";
        return;
    }
    //curBook->cost = QString::number(curBook->cost, 'f', 2).toDouble();

    refund = calcRefund(curBook->endDate, nextStart);
    qDebug() << "REFUNDING" << refund;


    newCost = curBook->cost + refund;
   // ui->editCost->setText(QString::number(newCost));



}
void MainWindow::setEditDayInfo(QDate date){
    std::pair<int, int> stayDays, usedDays, notusedDays;
    stayDays = monthDay(curBook->startDate, date);


    if(curBook->startDate > QDate::currentDate()){
        usedDays = std::make_pair(0,0);
        notusedDays = monthDay(curBook->startDate, curBook->endDate);
    }else{
        usedDays = monthDay(curBook->startDate, QDate::currentDate());
        notusedDays = monthDay(QDate::currentDate(), date);
    }
    int dLeft, mLeft;
    mLeft = stayDays.first - usedDays.first;
    dLeft = stayDays.second - usedDays.second;
    if(dLeft < 0)
        dLeft = 0;
    notusedDays = monthDay(QDate::currentDate(), date);
    ui->editDayUsed->setText(QString::number(usedDays.second));
    ui->editMonthUsed->setText(QString::number(usedDays.first));
    ui->editDaysRefunded->setText(QString::number(dLeft));
    ui->editMonthsRefunded->setText(QString::number(mLeft));
    ui->editDailyCost->setText("$"+QString::number(dCost, 'f',2));
    ui->editMonthlyCost->setText("$"+QString::number(mCost,'f',2));
    calcEditRefund(date);

}

void MainWindow::calcEditRefund(QDate date){
    int dUsed, mUsed, mRefund, dRefund;
    double adjustedDaily;
    double totalCost, dayCost, monthCost;
    double refAmt = 1;
    std::pair<int, int> curLength = monthDay(curBook->startDate, date);

    dUsed = ui->editDayUsed->text().toInt();
    mUsed = ui->editMonthUsed->text().toInt();
    dRefund = ui->editDaysRefunded->text().toInt();
    mRefund = ui->editMonthsRefunded->text().toInt();

    if(curBook->cost != editExpected){
        if(curBook->cost > editExpected){
            qDebug() << "Warning, booking more expensive than expected";
        }
      /* adjustedDaily = curBook->cost / curBook->stayLength;
       int numDays = date.toJulianDay() - curBook->startDate.toJulianDay();
       dayCost = numDays * adjustedDaily;
       monthCost = 0;
       */
        if(curBook->cost == 0){
            refAmt = 0;
        }
        else if(editExpected == 0){
            refAmt = 1;
        }
        else{
            refAmt = curBook->cost / editExpected;
            if(refAmt > 1)
                refAmt = 1;
        }

    }
    dayCost = curLength.second * dCost;
    if(dayCost > mCost)
        dayCost = mCost;
    monthCost = curLength.first * mCost;


    double refundCost;
    if(!checkNumber(ui->editCancel->text())){
        refundCost = 0;
    }
    else{

        refundCost = ui->editCancel->text().toDouble();
    }
    if(date > curBook->endDate){
        refundCost = 0;
    }
    totalCost = curBook->cost - ((dayCost + monthCost) * refAmt + refundCost);
    double labelCost = (dayCost + monthCost) * refAmt + refundCost;
    if(curBook->endDate >= date){
        if(totalCost < 0)
            totalCost = 0;
        if(labelCost > curBook->cost)
            labelCost = curBook->cost;
    }
    if(totalCost < 0){
        ui->editRefOwe->setText("Expected Amount Owed");
        ui->editRefundLabel->setText("Amount Owed");
        ui->editRefundAmt->setText(QString::number(totalCost * -1, 'f',2));
        ui->editRefundAmount->setText("$"+QString::number(totalCost * -1, 'f', 2));
    }
    else{
        ui->editRefOwe->setText("Expected Refund Amount");
        ui->editRefundLabel->setText("Refund Amount");
        ui->editRefundAmt->setText(QString::number(totalCost, 'f',2));
        ui->editRefundAmount->setText("$"+QString::number(totalCost, 'f', 2));

    }

    ui->editCost->setText(QString::number(labelCost, 'f', 2));
}

void MainWindow::on_editManagePayment_clicked()
{
    trans = new transaction();
    tNew = true;
    double owed;

    owed = ui->editRefundAmt->text().toDouble();
    bool type;
    ui->editRefundLabel->text() == "Refund" ? type = false : type = true;
    if(!type){
        owed *= -1;
    }
    QString note = "";
    payment * pay = new payment(this, trans, curClient->balance, owed , curClient, note, type, usernameLoggedIn, QString::number(currentshiftid));
    pay->exec();
    delete(pay);
}

void MainWindow::on_editCost_textChanged()
{

}

void MainWindow::on_editCancel_textChanged()
{

   if(!checkNumber(ui->editCancel->text())){
       return;
   }
   double refundAmt = ui->editCancel->text().toDouble();
   if(refundAmt < 0){
        ui->editCancel->setText("0");
        return;
   }
   dateChanger = true;
   calcEditRefund(ui->editDate->date());
}

void MainWindow::on_editRoom_clicked()
{
    QString tmpStyleSheet=this->styleSheet();
    this->setStyleSheet("");

   // swapper * swap = new Swapper();
    EditRooms * edit = new EditRooms(this, curBook, userLoggedIn, QString::number(currentshiftid), curClient);
    edit->exec();
    setBookSummary();
    ui->editOC->setText(QString::number(curBook->cost,'f',2));
    ui->editRefundAmt->setText("0");
    ui->editUpdate->setEnabled(false);
    ui->editRoomLabel->setText(curBook->room);
    delete(edit);

    this->setStyleSheet(tmpStyleSheet);

}
void MainWindow::doAlert(QString message){
    QString tmpStyleSheet=this->styleSheet();
    this->setStyleSheet("");

    QMessageBox::question(this, "Alert", message, QMessageBox::Ok);

    this->setStyleSheet(tmpStyleSheet);


}

void MainWindow::on_pushButton_bookRoom_clicked()
{
    addHistory(CLIENTLOOKUP);
    qDebug()<<"push book room";
    setSelectedClientInfo();
    if(!dbManager->checkDoubleBook(curClient->clientId))
    {

        if(!doMessageBox("Client has a current booking. Are you sure you wish to make another?"))
            return;
    }
    if(!dbManager->isBanned(curClient->clientId)){

        doMessageBox("User is currently restricted. Continue anyways?");
        /*
        if(!doMessageBox("User is currently restricted. Continue anyway?"))
            return;
            */
    }
    /*
    curClient = new Client();
    int nRow = ui->tableWidget_search_client->currentRow();
    if (nRow <0){
        if(curClientID == NULL)
            return;
        else{
            curClient->clientId = curClientID;
            curClient->fName = ui->label_cl_info_fName_val->text();
            curClient->mName = ui->label_cl_info_mName_val->text();
            curClient->lName =  ui->label_cl_info_lName_val->text();
            QString balanceString = ui->label_cl_info_balance_amt->text();
            balanceString.replace("$", "");
            curClient->balance =  balanceString.toFloat();
            curClient->fullName = QString(curClient->lName + ", " + curClient->fName + " " + curClient->mName);
            // curClient->fullName = QString(curClient->fName + " " + curClient->mName + " " + curClient->lName);
        }
    }
    else{
        curClientID = curClient->clientId = ui->tableWidget_search_client->item(nRow, 0)->text();
        curClient->fName =  ui->tableWidget_search_client->item(nRow, 1)->text();
        curClient->mName =  ui->tableWidget_search_client->item(nRow, 2)->text();
        curClient->lName =  ui->tableWidget_search_client->item(nRow, 3)->text();
        QString balanceString = ui->tableWidget_search_client->item(nRow, 5)->text();
        balanceString.replace("$", "");
        curClient->balance =  balanceString.toFloat();
        curClient->fullName = QString(curClient->lName + ", " + curClient->fName + " " + curClient->mName);
        // curClient->fullName = QString(curClient->fName + " " + curClient->mName + " " + curClient->lName);
    }



    // qDebug()<<"ID: " << curClientID << curClient->clientId;
    // qDebug()<<"NAME: " << curClient->fullName;
    // qDebug()<<"Balance: " << curClient->balance;
*/
    ui->stackedWidget->setCurrentIndex(BOOKINGLOOKUP);

}

void MainWindow::on_makeBookingButton_2_clicked()
{

    if(!checkNumber(ui->costInput->text()))
        return;

    backStack.clear();
    if(!doMessageBox("Finalize booking and add to database?")){
        return;
    }
    ui->actionBack->setEnabled(false);
    ui->makeBookingButton_2->setEnabled(false);

    curBook->lunch = "NULL";
    curBook->wakeTime = "NULL";
    QString month;
    if(curBook->monthly){
        month = "YES";
    }
    else{
        month = "NO";
    }
    double cost = QString::number(ui->costInput->text().toDouble(), 'f', 2).toDouble();
    QDate today = QDate::currentDate();
    QString values;
    QString todayDate = today.toString(Qt::ISODate);
    values = "'" + today.toString(Qt::ISODate) + "','" + curBook->stringStart + "','" + curBook->roomId + "','" +
             curClient->clientId + "','" + curBook->program + "','" + QString::number(cost) + "','" + curBook->stringStart
             + "','" + curBook->stringEnd + "','" + "YES'" + ",'" + month + "','" + curClient->fullName +"'";
//    QDate next = curBook->startDate;
    //QDate::fromString(ui->startLabel->text(), "yyyy-MM-dd");
    if(!dbManager->addBooking(curBook->stringStart, curBook->stringEnd, curBook->roomId, curBook->clientId, curClient->fullName, cost, curBook->program)){
        qDebug() << "THIS IS SO AWESOME";
        return;
    }

    curBook->cost = cost;
   // insertIntoBookingHistory(QString clientName, QString spaceId, QString program, QDate start, QDate end, QString action, QString emp, QString shift){
    qDebug()<<"check booking"<<curBook->roomId;

    if(!dbManager->updateBalance(curClient->balance - curBook->cost, curClient->clientId)){
        qDebug() << "ERROR ADDING TO BALANCE UPDATE";
    }
    if(!dbManager->insertIntoBookingHistory(curClient->fullName, curBook->room, curBook->program, curBook->stringStart, curBook->stringEnd, "NEW", usernameLoggedIn, QString::number(currentshiftid), curClient->clientId)){
        qDebug() << "ERROR INSERTING INTO BOOKING HISTORY";
    }

    ui->stackedWidget->setCurrentIndex(CONFIRMBOOKING);
    populateConfirm();

 }

void MainWindow::populateConfirm(){
    ui->confirmCost->setText(QString::number(curBook->cost, 'f', 2));
    ui->confirmEnd->setText(curBook->stringEnd);
    ui->confirmStart->setText(curBook->stringStart);
    ui->confirmLength->setText(QString::number(curBook->stayLength));
    if(curBook->monthly){
        ui->confirmMonthly->setText("YES");
    }else{
        ui->confirmMonthly->setText("NO");
    }
    ui->confirmRoom->setText(curBook->room);
    ui->confirmPaid->setText(QString::number((curClient->balance - curBook->cost), 'f', 2));
    ui->confirmProgram->setText(curBook->program);
    if(curBook->paidTotal < 0){
        ui->confirmPayRefund->setText("Total Refunded");
        ui->confirmTotalPaid->setText(QString::number(curBook->paidTotal * -1, 'f', 2));

    }else{
        ui->confirmPayRefund->setText("Total Paid");

        ui->confirmTotalPaid->setText(QString::number(curBook->paidTotal, 'f', 2));
    }

    //handle receipt

    ui->actionExport_to_PDF->setEnabled(true);  
//    MainWindow::on_actionExport_to_PDF_triggered();
    isRefund = curBook->paidTotal < 0;
    saveReceipt();
//    ui->actionReceipt->setEnabled(true);
//    createTextReceipt(ui->confirmCost->text(), transType, ui->confirmTotalPaid->text(), curBook->stringStart,
//                      curBook->stringEnd, ui->confirmLength->text(), true, isRefund);
}

//void MainWindow::on_monthCheck_stateChanged(int arg1)
//{

//}


//END COLIN ////////////////////////////////////////////////////////////////////



void MainWindow::on_EditUserButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(EDITUSERS);
    addHistory(ADMINPAGE);
    qDebug() << "pushed page " << ADMINPAGE;

}

void MainWindow::on_EditProgramButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(EDITPROGRAM);
    addHistory(ADMINPAGE);
    on_btn_listAllUsers_2_clicked();
    qDebug() << "pushed page " << ADMINPAGE;

}

void MainWindow::on_actionMain_Menu_triggered()
{
    addHistory(ui->stackedWidget->currentIndex());
    ui->stackedWidget->setCurrentIndex(MAINMENU);

    //reset client lookup buttons
    ui->pushButton_bookRoom->setEnabled(false);
    ui->pushButton_processPaymeent->setEnabled(false);
    ui->pushButton_editClientInfo->setEnabled(false);
    ui->pushButton_CaseFiles->setEnabled(false);
}


void MainWindow::on_pushButton_RegisterClient_clicked()
{
    addHistory(CLIENTLOOKUP);
    registerType = NEWCLIENT;

    curClientID = "";
    ui->stackedWidget->setCurrentIndex(CLIENTREGISTER);
    ui->label_cl_infoedit_title->setText("Register Client");
    ui->button_register_client->setText("Register");
    ui->dateEdit_cl_rulesign->setDate(QDate::currentDate());

    defaultRegisterOptions();
}

void MainWindow::on_pushButton_editClientInfo_clicked()
{
    addHistory(CLIENTLOOKUP);
    registerType = EDITCLIENT;

    getCurrentClientId();

    ui->stackedWidget->setCurrentIndex(CLIENTREGISTER);
    ui->label_cl_infoedit_title->setText("Edit Client Information");
    ui->button_register_client->setText("Save");
    //getCurrentClientId();
}


void MainWindow::on_reportsButton_clicked()
{
    if (dbManager->checkDatabaseConnection())
    {
        ui->btn_dailyReport->setChecked(true);
        ui->stackedWidget->setCurrentIndex(REPORTS);
        addHistory(MAINMENU);
        qDebug() << "pushed page " << MAINMENU;
    }
}


/*==============================================================================
SEARCH CLIENTS USING NAME
==============================================================================*/
//search client
void MainWindow::on_pushButton_search_client_clicked()
{
    qDebug() <<"START SEARCH CLIENT";
    ui->tabWidget_cl_info->setCurrentIndex(0);
    QString clientName = ui->lineEdit_search_clientName->text();

    useProgressDialog("Search Client "+clientName,  QtConcurrent::run(this, &searchClientListThread));
    statusBar()->showMessage(QString("Found " + QString::number(maxClient) + " Clients"), 5000);

    MainWindow::resizeTableView(ui->tableWidget_search_client);

    connect(ui->tableWidget_search_client, SIGNAL(cellClicked(int,int)), this, SLOT(set_curClient_name(int,int)),Qt::UniqueConnection);
    connect(ui->tableWidget_search_client, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(selected_client_info(int,int)),Qt::UniqueConnection);
}

void MainWindow::searchClientListThread(){

    QString clientName = ui->lineEdit_search_clientName->text();
    qDebug()<<"NAME: "<<clientName;
    QSqlQuery resultQ = dbManager->searchClientList(clientName);
    setup_searchClientTable(resultQ);
}

void MainWindow::on_checkBox_search_anonymous_clicked(bool checked)
{
    if(checked){
        QSqlQuery resultQ = dbManager->searchClientList("anonymous");
        setup_searchClientTable(resultQ);
        resizeTableView(ui->tableWidget_search_client);
     //  ui->lineEdit_search_clientName->
    }
    else
    {
        qDebug()<<"anonymous check not";
        initClientLookupInfo();
    }
    ui->pushButton_search_client->setEnabled(!checked);
}

void MainWindow::initClientLookupTable(){
    ui->tableWidget_search_client->setRowCount(0);

    ui->tableWidget_search_client->setColumnCount(6);
    ui->tableWidget_search_client->clear();

    ui->tableWidget_search_client->setHorizontalHeaderLabels(QStringList()<<"ClientID"<<"First Name"<<"Middle Name"<<"Last Name"<<"DateOfBirth"<<"Balance");



}

void MainWindow::set_curClient_name(int nRow, int nCol){
    Q_UNUSED(nCol)
    curClientName = "";
    if(nRow <0 && curClientID == NULL){
        ui->label_cl_curClient_title->hide();
        return;
    }
    ui->label_cl_curClient_title->show();
    QString fName, lName, mName;

    if(nRow <0){
        fName = ui->label_cl_info_fName_val->text();
        mName = ui->label_cl_info_mName_val->text();
        lName =  ui->label_cl_info_lName_val->text();
    }
    else{
            lName =  ui->tableWidget_search_client->item(nRow, 1)->text();
            fName =  ui->tableWidget_search_client->item(nRow, 2)->text();
            mName =  ui->tableWidget_search_client->item(nRow, 3)->text();
    }

    qDebug()<<"curClientName";
    //MAKE FULL NAME
    if(lName!=NULL)
        curClientName = QString(lName.toUpper());
    if(fName!=NULL){
        if(curClientName!="")
            curClientName += QString(", ");
        curClientName += QString(fName.toUpper());
    }
    if(mName!=NULL){
        if(curClientName!="")
            curClientName += QString(" ");
        curClientName += QString(mName.toUpper());
    }

    ui->label_cl_curClient->setText(curClientName);
}

//set up table widget to add result of search client using name
void MainWindow::setup_searchClientTable(QSqlQuery results){
    //initClientLookupInfo();


    ui->tableWidget_search_client->setRowCount(0);
     int colCnt = results.record().count();
    ui->tableWidget_search_client->setColumnCount(colCnt);
    ui->tableWidget_search_client->clear();

    ui->tableWidget_search_client->setHorizontalHeaderLabels(QStringList()<<"ClientID"<<"LastName"<<"FirstName"<<"Middle Initial"<<"DateOfBirth"<<"Balance");


    int row =0;
    while(results.next()){
        ui->tableWidget_search_client->insertRow(row);
        for(int i =0; i<colCnt; i++){
            if (i == colCnt - 1)
            {
                QString balance = QString("%1%2").arg(results.value(i).toDouble() >= 0 ? "$" : "-$").
                    arg(QString::number(fabs(results.value(i).toDouble()), 'f', 2));
                ui->tableWidget_search_client->
                    setItem(row, i, new QTableWidgetItem(balance));
            }
            else
            {
                ui->tableWidget_search_client->setItem(row, i, new QTableWidgetItem(results.value(i).toString()));

            }
            //qDebug() <<"row : "<<row << ", col: " << i << "item" << results.value(i).toString();
        }
        row++;
    }

    maxClient = row;
    //ui->tableWidget_search_client->show();
    ui->tableWidget_search_client->setColumnHidden(0, true);

}



//client information tab
void MainWindow::on_tabWidget_cl_info_currentChanged(int index)
{
    switch(index){
        case 0:
            qDebug()<<"Client information tab";
            break;

        case 1:
            if(curClientID == NULL || !newTrans)
                break;
            if(transacFuture.isRunning()|| !transacFuture.isFinished()){
                qDebug()<<"TransactionHistory Is RUNNING";
                return;
            }
             qDebug()<<"transaction info start really";
            ui->pushButton_cl_trans_more->setEnabled(true);
            transacFuture = QtConcurrent::run(this, &searchTransaction, curClientID);
            useProgressDialog("Read recent transaction...", transacFuture);
            //transacFuture.waitForFinished();
            if(ui->tableWidget_transaction->rowCount()>= transacTotal)
                ui->pushButton_cl_trans_more->setEnabled(false);
            qDebug()<<"client Transaction list";
            resizeTableView(ui->tableWidget_transaction);
            newTrans = false;
            break;

       case 2:
            if(curClientID == NULL || !newHistory)
                break;
             if(bookHistoryFuture.isRunning()|| !bookHistoryFuture.isFinished()){
                 qDebug()<<"BookingHistory Is RUNNING";
                 return;
             }
                        ui->pushButton_cl_book_more->setEnabled(true);
             bookHistoryFuture = QtConcurrent::run(this, &searchBookHistory, curClientID);
             useProgressDialog("Read recent booking...", bookHistoryFuture);
             //bookHistoryFuture.waitForFinished();
             if(ui->tableWidget_booking->rowCount() >= bookingTotal)
                 ui->pushButton_cl_book_more->setEnabled(false);
             resizeTableView(ui->tableWidget_booking);
             newHistory = false;
             break;
    case 3:
       setStorageClient();
        break;

    case 4:
        if(curClientID == NULL || !newReceipt) break;
        ui->btn_displayReceipt->setEnabled(false);
        QFuture<void> receiptFuture = QtConcurrent::run(this, &searchReceipts, curClientID, ui->tw_receipts);
        useProgressDialog("Loading receipts...", receiptFuture);
        resizeTableView(ui->tw_receipts);
        newReceipt = false;
    }
}

void MainWindow::setStorageClient(){
    ui->clientStorage->clear();
    if(ui->tableWidget_search_client->selectionModel()->currentIndex().row() == -1)
        return;
    if(curClientID == "" || curClientID == NULL)
        return;
    QSqlQuery result = dbManager->loadStorage(curClientID);
    if(result.next()){
        QString res = result.value("StorageItems").toString();
        ui->clientStorage->setPlainText(res);
    }
    ui->clientStorage->setEnabled(false);
}

//get client information after searching
//double click client information
void MainWindow::selected_client_info(int nRow, int nCol)
{
    Q_UNUSED(nCol)

    if(displayFuture.isRunning()|| !displayFuture.isFinished()){
        qDebug()<<"ProfilePic Is RUNNING";
        return;
        //displayFuture.cancel();
    }
    if(displayPicFuture.isRunning() || !displayPicFuture.isFinished()){
        qDebug()<<"ProfilePic Is RUNNING";
         return;
       // displayPicFuture.cancel();
    }
//    //clear scene
//    QGraphicsScene *scene = new QGraphicsScene();
//    scene->clear();
//    ui->graphicsView_getInfo->setScene(scene);

    curClientID = ui->tableWidget_search_client->item(nRow, 0)->text();
    ui->textEdit_cl_info_comment->clear();
    newTrans = true;
    newHistory = true;
    newReceipt = true;
    getClientInfo();
    initClTransactionTable();
    initClBookHistoryTable();
    initClReceiptTable(ui->tw_receipts);
}


void MainWindow::getClientInfo(){

    ui->tabWidget_cl_info->setCurrentIndex(0);
    transacNum = 5;
    bookingNum = 5;
    transacTotal = dbManager->countInformationPerClient("Transac", curClientID);
    bookingTotal = dbManager->countInformationPerClient("booking", curClientID);


    displayFuture = QtConcurrent::run(this, &displayClientInfoThread, curClientID);
    useProgressDialog("Read Client Information...", displayFuture);
    //displayFuture.waitForFinished();
    statusColor();
    displayPicFuture = QtConcurrent::run(this, &displayPicThread);
    useProgressDialog("Read Client Picture...", displayPicFuture);
    addInfoPic(profilePic);
    //displayPicFuture.waitForFinished();

    //useProgressDialog("Read Client Profile Picture...", displayPicFuture);
}

//change status background color after reading client information
void MainWindow::statusColor(){
    QPalette pal(ui->label_cl_info_status->palette());
    QString clStatus = ui->label_cl_info_status->text().toLower();
    if(clStatus == "green"){
        ui->label_cl_info_status->setText("Good Standing");
        pal.setColor(QPalette::Normal, QPalette::Background, Qt::green);
    }else if(clStatus == "yellow"){
        ui->label_cl_info_status->setText("Restricted Access");
        pal.setColor(QPalette::Normal, QPalette::Background, Qt::yellow);
    }else if(clStatus == "red"){
        ui->label_cl_info_status->setText("NO Access");
        pal.setColor(QPalette::Normal, QPalette::Background, Qt::red);
    }else{
        ui->label_cl_info_status->setText("");
        ui->label_cl_info_status->setAutoFillBackground(false);
        return;
    }
    ui->label_cl_info_status->setPalette(pal);
    ui->label_cl_info_status->setAutoFillBackground(true);
}

//delete client info picture  (not use currently)
void MainWindow::clientSearchedInfo(){
    QGraphicsScene *scene = new QGraphicsScene();
    scene->clear();
    ui->graphicsView_getInfo->setScene(scene);

}

//thread function to add client information to the form
void MainWindow::displayClientInfoThread(QString val){

    qDebug()<<"DISPLAY THREAD: " <<val;

    QSqlQuery clientInfo = dbManager->searchClientInfo(val);

   clientInfo.next();

   QString balance = QString("%1%2").arg(clientInfo.value(4).toDouble() >= 0 ? "$" : "-$").
                    arg(QString::number(fabs(clientInfo.value(4).toDouble()), 'f', 2));

   ui->label_cl_info_fName_val->setText(clientInfo.value(0).toString());
   ui->label_cl_info_mName_val->setText(clientInfo.value(1).toString());
   ui->label_cl_info_lName_val->setText(clientInfo.value(2).toString());
   ui->label_cl_info_dob_val->setText(clientInfo.value(3).toString());
   ui->label_cl_info_balance_amt->setText(balance);
   ui->label_cl_info_sin_val->setText(clientInfo.value(5).toString());
   ui->label_cl_info_gaNum_val->setText(clientInfo.value(6).toString());
   QString caseWorkerName = caseWorkerList.key(clientInfo.value(7).toInt());
   ui->label_cl_info_caseWorker_val->setText(caseWorkerName);
   ui->label_cl_info_ruleSignDate_val->setText(clientInfo.value(8).toString());
   ui->label_cl_info_status->setText(clientInfo.value(9).toString());

   ui->label_cl_info_nok_name_val->setText(clientInfo.value(10).toString());
   ui->label_cl_info_nok_relationship_val->setText(clientInfo.value(11).toString());
   ui->label_cl_info_nok_loc_val->setText(clientInfo.value(12).toString());
   ui->label_cl_info_nok_contatct_val->setText(clientInfo.value(13).toString());

   ui->label_cl_info_phys_name_val->setText(clientInfo.value(14).toString());
   ui->label_cl_info_phys_contact_val->setText(clientInfo.value(15).toString());

   ui->label_cl_info_Supporter_name_val->setText(clientInfo.value(16).toString());
   ui->label_cl_info_Supporter_contact_val->setText(clientInfo.value(17).toString());

   ui->label_cl_info_Supporter2_name_val->setText(clientInfo.value(18).toString());
   ui->label_cl_info_Supporter2_contact_val->setText(clientInfo.value(19).toString());
   ui->textEdit_cl_info_comment->document()->setPlainText(clientInfo.value(20).toString());
   ui->lbl_espDays->setText(clientInfo.value(21).toString());
}

void MainWindow::displayPicThread()
{
    qDebug()<<"displayPicThread";

    if(!dbManager->searchClientInfoPic(&profilePic, curClientID)){
            qDebug()<<"ERROR to get pic";
            return;
        }
    qDebug()<<"Add picture";

//    addInfoPic(profilePic);
}

//add image to client info picture graphicview
void MainWindow::addInfoPic(QImage img){
    qDebug()<<"Add Info Picture??";
    qDebug()<<"IMAGE SIZE"<<img.byteCount();
    QPixmap item2 = QPixmap::fromImage(img);
    QPixmap scaled = QPixmap(item2.scaledToWidth((int)(ui->graphicsView_getInfo->width()*0.9), Qt::SmoothTransformation));
    QGraphicsScene *scene2 = new QGraphicsScene();
    scene2->addPixmap(QPixmap(scaled));
    ui->graphicsView_getInfo->setScene(scene2);
    ui->graphicsView_getInfo->show();
}

//create new client for booking
void MainWindow::setSelectedClientInfo(){
    //transacTotal
    curClient = new Client();
    cNew = true;
    int nRow = ui->tableWidget_search_client->currentRow();
    if (nRow <0){
        if(curClientID == NULL){
            statusBar()->showMessage(tr("Please search and select Client"), 5000);
            return;
        }
        else{
            curClient->clientId = curClientID;
            curClient->fName = ui->label_cl_info_fName_val->text();
            curClient->mName = ui->label_cl_info_mName_val->text();
            curClient->lName =  ui->label_cl_info_lName_val->text();
            QString balanceString = ui->label_cl_info_balance_amt->text();
            balanceString.replace("$", "");
            curClient->balance =  balanceString.toFloat();
            // curClient->balance =  ui->label_cl_info_balance_amt->text().toFloat();

         //   curClient->fullName = QString(curClient->lName + " " + curClient->fName + " " + curClient->mName);
        }
    }
    else{
        curClientID = curClient->clientId = ui->tableWidget_search_client->item(nRow, 0)->text();
        curClient->lName =  ui->tableWidget_search_client->item(nRow, 1)->text();
        curClient->fName =  ui->tableWidget_search_client->item(nRow, 2)->text();
        curClient->mName =  ui->tableWidget_search_client->item(nRow, 3)->text();
        QString balanceString = ui->tableWidget_search_client->item(nRow, 5)->text();
        balanceString.replace("$", "");
        curClient->balance =  balanceString.toFloat();
        // curClient->balance =  ui->tableWidget_search_client->item(nRow, 5)->text().toFloat();

      //  curClient->fullName = QString(curClient->lName + " " + curClient->fName + " " + curClient->mName);
    }

    //MAKE FULL NAME
    if(curClient->lName!=NULL)
        curClient->fullName = QString(curClient->lName);
    if(curClient->fName!=NULL){
        if(curClient->fullName!="")
            curClient->fullName += QString(", ");
        curClient->fullName += QString(curClient->fName);
    }
    if(curClient->mName!=NULL){
        if(curClient->fullName!="")
            curClient->fullName += QString(" ");
        curClient->fullName += QString(curClient->mName);
    }

    qDebug()<<"ID: " << curClientID << curClient->clientId;
    qDebug()<<"NAME: " << curClient->fullName;
    qDebug()<<"Balance: " << curClient->balance;
}

//search transaction list when click transaction list
void MainWindow::searchTransaction(QString clientId){
    qDebug()<<"search transaction STaRt";

    QSqlQuery transQuery = dbManager->searchClientTransList(transacNum, clientId, CLIENTLOOKUP);
    //initClientLookupInfo();
    initClTransactionTable();
    qDebug()<<"############CHECK TRANS QUERY";
    displayTransaction(transQuery, ui->tableWidget_transaction);

    QString totalNum= (transacTotal == 0)? "0 / 0" :
                     (QString::number(ui->tableWidget_transaction->rowCount()) + " / " + QString::number(transacTotal));
    ui->label_cl_trans_total_num->setText(totalNum + " Transaction(s)");

    qDebug()<<"FINE FOR SEARCH clientTransaction";
}

void MainWindow::initClTransactionTable(){
    ui->tableWidget_transaction->setRowCount(0);

    ui->tableWidget_transaction->setColumnCount(8);
    ui->tableWidget_transaction->clear();

    ui->tableWidget_transaction->setHorizontalHeaderLabels(QStringList()<<"Date"<<"Type"<<"Amount"<<"Payment"<<"Employee"<<"Cheque No"<<"MSD"<<"Cheque Date");
    ui->tableWidget_transaction->setMinimumHeight(30*6-1);

}


//search transaction list when click transaction list
void MainWindow::displayTransaction(QSqlQuery results, QTableWidget* table){
    qDebug()<<"display transaction table";
   // ui->textEdit_cl_info_comment->clear();
    int row = 0;
    int colCnt = results.record().count();
    while(results.next()){
        table->insertRow(row);
        for(int i =0; i<colCnt; i++){

            if(i == 2){
                QString balance = QString("%1%2").arg(results.value(i).toDouble() >= 0 ? "$" : "-$").
                    arg(QString::number(fabs(results.value(i).toDouble()), 'f', 2));
                table->setItem(row, i, new QTableWidgetItem(balance));
            }
            else
                table->setItem(row, i, new QTableWidgetItem(results.value(i).toString()));
            //qDebug() <<"row : "<<row << ", col: " << i << "item" << results.value(i).toString();
        }

        row++;
    }

    if( table == ui->tableWidget_casefile_transaction)
        return;
    while(row > transacTotal){
        table->setRowCount(transacTotal);
    }
    if (row > 23){
        table->setMinimumHeight(30*24 -1);
        return;
    }
    table->setMinimumHeight(30*(row+1) -1);
}
//get more transaction list for client info tab
void MainWindow::on_pushButton_cl_trans_more_clicked()
{
    transacNum +=5;
    searchTransaction(curClientID);
    if(ui->tableWidget_transaction->rowCount() >= transacTotal)
        ui->pushButton_cl_trans_more->setEnabled(false);
    resizeTableView(ui->tableWidget_transaction);
}

//search booking history when click booking history tab
void MainWindow::searchBookHistory(QString clientId){
    qDebug()<<"search booking";

    QSqlQuery bookingQuery = dbManager->searchBookList(bookingNum, clientId, CLIENTLOOKUP);
    displayBookHistory(bookingQuery, ui->tableWidget_booking);
   // dbManager->printAll(bookingQuery);

    QString totalNum = (bookingTotal == 0)? "0 / 0" :
                        QString::number(ui->tableWidget_booking->rowCount()) + " / " + QString::number(bookingTotal);
    ui->label_cl_booking_total_num->setText(totalNum + " Booking(s)");
}

//initialize client booking history table in client info tab
void MainWindow::initClBookHistoryTable(){
    ui->tableWidget_booking->setRowCount(0);
    ui->tableWidget_booking->setColumnCount(6);
    ui->tableWidget_booking->clear();
    ui->tableWidget_booking->setHorizontalHeaderLabels(QStringList()<<"Booking Date"<<"Program"<<"Start Date"<< "Checkout Date"<<"Cost"<<"Space No");
    ui->tableWidget_booking->setMinimumHeight(30*6-1);
}


//display booking history in the table view
void MainWindow::displayBookHistory(QSqlQuery results, QTableWidget * table){
    initClBookHistoryTable();
    int colCnt = results.record().count();
    int row =table->rowCount();
    while(results.next()){
        table->insertRow(row);
        for(int i =0; i<colCnt; i++){
            if(i == 4){
                QString balance = QString("%1%2").arg(results.value(i).toDouble() >= 0 ? "$" : "-$").
                    arg(QString::number(fabs(results.value(i).toDouble()), 'f', 2));
                table->setItem(row, i, new QTableWidgetItem(balance));
            }
            else
                table->setItem(row, i, new QTableWidgetItem(results.value(i).toString()));
            //qDebug() <<"row : "<<row << ", col: " << i << "item" << results.value(i).toString();
        }
        row++;
    }
    if(table == ui->tableWidget_casefile_booking)
        return;
    if(row > bookingTotal)
        table->setRowCount(bookingTotal);

    if (row > 23){
        table->setMinimumHeight(30*24 -1);
        return;
    }
    if(row >5)
        table->setMinimumHeight(30*(row+1) -1);
}

//click more client button fore booking history
void MainWindow::on_pushButton_cl_book_more_clicked()
{
    bookingNum +=5;
    searchBookHistory(curClientID);
    if(ui->tableWidget_booking->rowCount() >= bookingTotal )
        ui->pushButton_cl_book_more->setEnabled(false);
    resizeTableView(ui->tableWidget_booking);
}

//DELETE CLIENT INFORMATION FIELDS
void MainWindow::initClientLookupInfo(){
    //init client search table
    if(ui->tableWidget_search_client->columnCount()>0){
        ui->tableWidget_search_client->setColumnCount(0);
        ui->tableWidget_search_client->clear();
        ui->lineEdit_search_clientName->clear();
    }
    ui->label_cl_curClient->setText(curClientName);
    //init client Info Form Field
    ui->label_cl_info_fName_val->clear();
    ui->label_cl_info_mName_val->clear();
    ui->label_cl_info_lName_val->clear();
    ui->label_cl_info_dob_val->clear();
    ui->label_cl_info_balance_amt->clear();
    ui->label_cl_info_sin_val->clear();
    ui->label_cl_info_gaNum_val->clear();
    ui->label_cl_info_caseWorker_val->clear();
    ui->label_cl_info_ruleSignDate_val->clear();
    ui->label_cl_info_status->clear();

    ui->label_cl_info_nok_name_val->clear();
    ui->label_cl_info_nok_relationship_val->clear();
    ui->label_cl_info_nok_loc_val->clear();
    ui->label_cl_info_nok_contatct_val->clear();

    ui->label_cl_info_phys_name_val->clear();
    ui->label_cl_info_phys_contact_val->clear();

    ui->label_cl_info_Supporter_name_val->clear();
    ui->label_cl_info_Supporter_contact_val->clear();
    ui->label_cl_info_Supporter2_name_val->clear();
    ui->label_cl_info_Supporter2_contact_val->clear();

    ui->textEdit_cl_info_comment->clear();
    ui->lbl_espDays->clear();

    qDebug()<<"CLEAR ALL INFO FIELD";
    QGraphicsScene *scene = new QGraphicsScene();
    scene->clear();
    ui->graphicsView_getInfo->setScene(scene);

    profilePic = (QImage)NULL;
    qDebug()<<"CLEAR IMAGE FIELD";
    ui->label_cl_info_status->setAutoFillBackground(false);

    //initialize transaction
    initClTransactionTable();

    //initialize transaction total count
    transacTotal = 0;
    ui->label_cl_trans_total_num->setText("");

    //initialize booking history table
    bookingTotal = 0;
    initClBookHistoryTable();

    //initialize booking total count
    ui->label_cl_booking_total_num->setText("");

    qDebug()<<"START BUTTON SETTUP";
    //disable buttons that need a clientId
    if(curClientID == NULL){
        qDebug()<<"CurClient ID not existing";
        ui->pushButton_bookRoom->setEnabled(false);
        ui->pushButton_processPaymeent->setEnabled(false);
        ui->pushButton_editClientInfo->setEnabled(false);
        ui->pushButton_CaseFiles->setEnabled(false);
    }
    qDebug()<<"START HIDE BUTTON SETTUP";
    //hide buttons for different workflows

            //     ui->hs_brpp->changeSize(13,20, QSizePolicy::Fixed, QSizePolicy::Fixed);
            // ui->hs_ppcf->changeSize(13,20, QSizePolicy::Fixed, QSizePolicy::Fixed);
            // ui->hs_cfec->changeSize(13,20, QSizePolicy::Fixed, QSizePolicy::Fixed);
    qDebug()<<"WORK FLOW: " << workFlow;
    switch (workFlow){
    case BOOKINGPAGE:
        ui->pushButton_CaseFiles->setVisible(false);
        ui->pushButton_processPaymeent->setVisible(false);
        ui->pushButton_bookRoom->setVisible(true);
        ui->hs_brpp->changeSize(13,20,QSizePolicy::Fixed,QSizePolicy::Fixed);
        ui->hs_ppcf->changeSize(0,0,QSizePolicy::Fixed,QSizePolicy::Fixed);
        ui->hs_cfec->changeSize(0,0,QSizePolicy::Fixed,QSizePolicy::Fixed);

        // ui->hs_brpp->changeSize(1,1,QSizePolicy::Expanding,QSizePolicy::Fixed);
        // ui->hs_ppcf->changeSize(1,1,QSizePolicy::Fixed,QSizePolicy::Fixed);
        // ui->hs_cfec->changeSize(1,1,QSizePolicy::Fixed,QSizePolicy::Fixed);
        break;
    case PAYMENTPAGE:
        ui->pushButton_CaseFiles->setVisible(false);
        ui->pushButton_bookRoom->setVisible(false);
        ui->pushButton_processPaymeent->setVisible(true);

        ui->hs_brpp->changeSize(0,0,QSizePolicy::Fixed,QSizePolicy::Fixed);
        ui->hs_ppcf->changeSize(13,20,QSizePolicy::Fixed,QSizePolicy::Fixed);
        ui->hs_cfec->changeSize(0,0,QSizePolicy::Fixed,QSizePolicy::Fixed);

        // ui->hs_brpp->changeSize(1,1,QSizePolicy::Fixed,QSizePolicy::Fixed);
        // ui->hs_ppcf->changeSize(1,1,QSizePolicy::Expanding,QSizePolicy::Fixed);
        // ui->hs_cfec->changeSize(1,1,QSizePolicy::Fixed,QSizePolicy::Fixed);
        break;
    case CASEFILE:
        ui->pushButton_CaseFiles->setVisible(true);
        ui->pushButton_bookRoom->setVisible(false);
        ui->pushButton_processPaymeent->setVisible(false);
        ui->hs_brpp->changeSize(0,0,QSizePolicy::Fixed,QSizePolicy::Fixed);
        ui->hs_ppcf->changeSize(0,0,QSizePolicy::Fixed,QSizePolicy::Fixed);
        ui->hs_cfec->changeSize(13,20,QSizePolicy::Fixed,QSizePolicy::Fixed);

        // ui->hs_brpp->changeSize(1,1,QSizePolicy::Fixed,QSizePolicy::Fixed);
        // ui->hs_ppcf->changeSize(1,1,QSizePolicy::Fixed,QSizePolicy::Fixed);
        // ui->hs_cfec->changeSize(1,1,QSizePolicy::Expanding,QSizePolicy::Fixed);
        break;
    case CLIENTLOOKUP:
        ui->pushButton_CaseFiles->setVisible(true);
        ui->pushButton_processPaymeent->setVisible(true);
        ui->pushButton_bookRoom->setVisible(true);
        ui->hs_brpp->changeSize(13,20,QSizePolicy::Fixed,QSizePolicy::Fixed);
        ui->hs_ppcf->changeSize(13,20,QSizePolicy::Fixed,QSizePolicy::Fixed);
        ui->hs_cfec->changeSize(13,20,QSizePolicy::Fixed,QSizePolicy::Fixed);
        ui->horizontalLayout_15->update();
        break;
    }

    qDebug()<<"END BUTTON SETTUP";
}


/*===================================================================
  REGISTRATION PAGE
  ===================================================================*/

void MainWindow::getCurrentClientId(){
    int nRow = ui->tableWidget_search_client->currentRow();
    if (nRow <0)
        return;
    curClientID = ui->tableWidget_search_client->item(nRow, 0)->text();

}

//Client Regiter widget [TAKE A PICTURE] button
void MainWindow::on_button_cl_takePic_clicked()
{
    TakePhoto *camDialog = new TakePhoto();

    connect(camDialog, SIGNAL(showPic(QImage)), this, SLOT(addPic(QImage)));
    camDialog->show();
}

//delete client picture
void MainWindow::on_button_cl_delPic_clicked()
{
    QGraphicsScene *scene = new QGraphicsScene();
    scene->clear();
    profilePic = (QImage)NULL;
    ui->graphicsView_cl_pic->setScene(scene);

    //delete picture function to database

}
//upload picture from file
void MainWindow::on_button_cl_uploadpic_clicked()
{
    QString strFilePath = MainWindow::browse();

    if (!strFilePath.isEmpty())
    {
        QImage img(strFilePath);
        profilePic = img.scaledToWidth(300);
        addPic(profilePic);
    }
    else
    {
        qDebug() << "Empty file path";
    }
}

/*------------------------------------------------------------------
  add picture into graphicview (after taking picture in pic dialog
  ------------------------------------------------------------------*/
void MainWindow::addPic(QImage pict){

  //  qDebug()<<"ADDPIC";
    profilePic = pict.copy();
    QPixmap item = QPixmap::fromImage(pict);
    QPixmap scaled = QPixmap(item.scaledToWidth((int)(ui->graphicsView_cl_pic->width()*0.9), Qt::SmoothTransformation));
    QGraphicsScene *scene = new QGraphicsScene();
    scene->addPixmap(QPixmap(scaled));
    ui->graphicsView_cl_pic->setScene(scene);
    ui->graphicsView_cl_pic->show();
}



void MainWindow::on_button_clear_client_regForm_clicked()
{
    clear_client_register_form();
}
void MainWindow::on_checkBox_cl_dob_no_clicked(bool checked)
{
        ui->dateEdit_cl_dob->setEnabled(!checked);
}

void MainWindow::getListRegisterFields(QStringList* fieldList)
{
    QString caseWorkerId = QString::number(caseWorkerList.value(ui->comboBox_cl_caseWorker->currentText()));
    if(caseWorkerId == "0")
        caseWorkerId = "";

    QString firstName = ui->lineEdit_cl_fName->text();
    QString middleName = ui->lineEdit_cl_mName->text();
    QString lastName = ui->lineEdit_cl_lName->text();

    if (!firstName.isEmpty())
        firstName[0] = firstName[0].toUpper();
    if (!middleName.isEmpty())
        middleName[0] = middleName[0].toUpper();
    if (!lastName.isEmpty())
        lastName[0] = lastName[0].toUpper();

    *fieldList << firstName
               << middleName
               << lastName
               << (ui->checkBox_cl_dob_no->isChecked()? "" : ui->dateEdit_cl_dob->date().toString("yyyy-MM-dd"))
               << ui->lineEdit_cl_SIN->text()
               << ui->lineEdit_cl_GANum->text()
               << caseWorkerId   //QString::number(caseWorkerList.value(ui->comboBox_cl_caseWorker->currentText())) //grab value from case worker dropdown I don't know how to do it
               << ui->dateEdit_cl_rulesign->date().toString("yyyy-MM-dd")
               << ui->lineEdit_cl_nok_name->text()
               << ui->lineEdit_cl_nok_relationship->text()
               << ui->lineEdit_cl_nok_loc->text()
               << ui->lineEdit_cl_nok_ContactNo->text()
               << ui->lineEdit_cl_phys_name->text()
               << ui->lineEdit_cl_phys_ContactNo->text()
               << ui->lineEdit_cl_supporter_Name->text()
               << ui->lineEdit_cl_supporter_ContactNo->text()
               << ui->lineEdit_cl_supporter2_Name->text()
               << ui->lineEdit_cl_supporter2_ContactNo->text()
               << ui->comboBox_cl_status->currentText() //grab value from status dropdown
               << ui->plainTextEdit_cl_comments->toPlainText();

}

void MainWindow::getRegisterLogFields(QStringList* fieldList)
{
    QString firstName = ui->lineEdit_cl_fName->text();
    QString middleName = ui->lineEdit_cl_mName->text();
    QString lastName = ui->lineEdit_cl_lName->text();
    QString fullName = "";

    if (!lastName.isEmpty())
    {
        lastName[0] = lastName[0].toUpper();
        fullName += QString(lastName + ", ");
    }
    if (!firstName.isEmpty())
    {
        firstName[0] = firstName[0].toUpper();
        fullName += QString(firstName + " ");
    }
    if (!middleName.isEmpty())
    {
        middleName[0] = middleName[0].toUpper();
        fullName += QString(middleName + " ");
    }

    QString action;
    if(registerType == NEWCLIENT || ui->button_register_client->text() == "Register")
        action = "Registered";
    else if(registerType == EDITCLIENT)
        action = "Updated";
    else if(registerType == DELETECLIENT)
        action = "Deleted";
    else
        return;

    *fieldList << fullName
               << action
               << QDate::currentDate().toString("yyyy-MM-dd")
               << QString::number(currentshiftid) // get shift number
               << QTime::currentTime().toString("hh:mm:ss")
               << usernameLoggedIn; //employee name

}

void MainWindow::clear_client_register_form(){
    ui->lineEdit_cl_fName->clear();
    ui->lineEdit_cl_mName->clear();
    ui->lineEdit_cl_lName->clear();
    ui->lineEdit_cl_SIN->clear();
    ui->lineEdit_cl_GANum->clear();
    ui->comboBox_cl_caseWorker->setCurrentIndex(0);
    ui->lineEdit_cl_nok_name->clear();
    ui->lineEdit_cl_nok_relationship->clear();
    ui->lineEdit_cl_nok_loc->clear();
    ui->lineEdit_cl_nok_ContactNo->clear();
    ui->lineEdit_cl_phys_name->clear();
    ui->lineEdit_cl_phys_ContactNo->clear();
    ui->lineEdit_cl_supporter_Name->clear();
    ui->lineEdit_cl_supporter_ContactNo->clear();
    ui->lineEdit_cl_supporter2_Name->clear();
    ui->lineEdit_cl_supporter2_ContactNo->clear();

    ui->comboBox_cl_status->setCurrentIndex(0);
    ui->plainTextEdit_cl_comments->clear();
    QDate defaultDob= QDate::fromString("1990-01-01","yyyy-MM-dd");
    ui->dateEdit_cl_dob->setDate(defaultDob);
    ui->checkBox_cl_dob_no->setChecked(false);
    ui->dateEdit_cl_dob->setEnabled(true);
    ui->dateEdit_cl_rulesign->setDate(QDate::currentDate());

    QPalette pal;
    pal.setColor(QPalette::Normal, QPalette::WindowText, Qt::black);
    ui->label_cl_fName->setPalette(pal);
    ui->label_cl_mName->setPalette(pal);
    ui->label_cl_lName->setPalette(pal);


    on_button_cl_delPic_clicked();
}

//read client information to edit
void MainWindow::read_curClient_Information(QString ClientId){
    QSqlQuery clientInfo = dbManager->searchTableClientInfo("Client", ClientId);
//    dbManager->printAll(clientInfo);
    clientInfo.next();

    //input currentValue;

    qDebug()<<"FNAme: "<<clientInfo.value(1).toString()<<"MNAme: "<<clientInfo.value(2).toString()<<"LNAME: "<<clientInfo.value(3).toString();
    qDebug()<<"DOB: "<<clientInfo.value(4).toString() <<"GANUM: "<<clientInfo.value(6).toString()<<"SIN: "<<clientInfo.value(7).toString();

    ui->lineEdit_cl_fName->setText(clientInfo.value(1).toString());

    ui->lineEdit_cl_mName->setText(clientInfo.value(2).toString());
    ui->lineEdit_cl_lName->setText(clientInfo.value(3).toString());

    if(clientInfo.value(4).toString() == ""){
        ui->checkBox_cl_dob_no->setChecked(true);
        ui->dateEdit_cl_dob->setEnabled(false);
    }
    else{
        ui->checkBox_cl_dob_no->setChecked(false);
        ui->dateEdit_cl_dob->setEnabled(true);
        ui->dateEdit_cl_dob->setDate(QDate::fromString(clientInfo.value(4).toString(),"yyyy-MM-dd"));
    }
    QString caseWorkerName = caseWorkerList.key(clientInfo.value(21).toInt());
    //balnace?
    ui->comboBox_cl_caseWorker->setCurrentText(caseWorkerName);
    ui->lineEdit_cl_SIN->setText(clientInfo.value(6).toString());
    ui->lineEdit_cl_GANum->setText(clientInfo.value(7).toString());
    ui->dateEdit_cl_rulesign->setDate(QDate::fromString(clientInfo.value(8).toString(),"yyyy-MM-dd"));

    //NEXT OF KIN FIELD
    ui->lineEdit_cl_nok_name->setText(clientInfo.value(9).toString());
    ui->lineEdit_cl_nok_relationship->setText(clientInfo.value(10).toString());
    ui->lineEdit_cl_nok_loc->setText(clientInfo.value(11).toString());
    ui->lineEdit_cl_nok_ContactNo->setText(clientInfo.value(12).toString());

    //Physician
    ui->lineEdit_cl_phys_name->setText(clientInfo.value(13).toString());
    ui->lineEdit_cl_phys_ContactNo->setText(clientInfo.value(14).toString());

    //Supporter
    ui->lineEdit_cl_supporter_Name->setText(clientInfo.value(15).toString());
    ui->lineEdit_cl_supporter_ContactNo->setText(clientInfo.value(16).toString());
    ui->lineEdit_cl_supporter2_Name->setText(clientInfo.value(22).toString());
    ui->lineEdit_cl_supporter2_ContactNo->setText(clientInfo.value(23).toString());

    ui->comboBox_cl_status->setCurrentText(clientInfo.value(17).toString());

    //comments
    ui->plainTextEdit_cl_comments->clear();
    ui->plainTextEdit_cl_comments->setPlainText(clientInfo.value(18).toString());


    //picture
    QByteArray data = clientInfo.value(20).toByteArray();
    QImage profile = QImage::fromData(data, "PNG");
    addPic(profile);

}

//Client information input and register click
void MainWindow::on_button_register_client_clicked()
{

    if (MainWindow::check_client_register_form())
    {
        QStringList registerFieldList, logFieldList;
        MainWindow::getListRegisterFields(&registerFieldList);
        MainWindow::getRegisterLogFields(&logFieldList);
        if(!dbManager->insertClientLog(&logFieldList))
            return;

        if(registerType == NEWCLIENT || ui->label_cl_infoedit_title->text() == "Register Client")
        {
            bool tmp = check_unique_client();
            if(tmp || ignore_duplicate){
                if (dbManager->insertClientWithPic(&registerFieldList, &profilePic))
                {
                    statusBar()->showMessage("Client Registered Sucessfully.", 5000);
                    qDebug() << "Client registered successfully";

                    clear_client_register_form();
                    ui->stackedWidget->setCurrentIndex(1);
                }
                else
                {
                  qDebug() << tmp << ignore_duplicate;
                    statusBar()->showMessage("Register Failed. Check information.", 5000);
                    qDebug() << "Could not register client";
                }
            }

        }
        else if(registerType == EDITCLIENT)
        {
            if (dbManager->updateClientWithPic(&registerFieldList, curClientID, &profilePic))
            {
                statusBar()->showMessage("Client information updated Sucessfully.");
                qDebug() << "Client info edit successfully";
                clear_client_register_form();
                ui->stackedWidget->setCurrentIndex(1);
            }
            else
            {
                statusBar()->showMessage("Register Failed. Check information.");
                qDebug() << "Could not edit client info";
            }
        }
        else
            return;

    }
    else
    {
        statusBar()->showMessage("Register Failed. Check information.");
        qDebug() << "Register form check was false";
    }
}


//check if the value is valid or not
bool MainWindow::check_client_register_form(){
    if(ui->lineEdit_cl_fName->text().isEmpty()
            && ui->lineEdit_cl_mName->text().isEmpty()
            && ui->lineEdit_cl_lName->text().isEmpty()){
        statusBar()->showMessage(QString("Please Enter Name of Clients"), 5000);
        QPalette pal;
        pal.setColor(QPalette::Normal, QPalette::WindowText, Qt::red);
        ui->label_cl_fName->setPalette(pal);
        ui->label_cl_mName->setPalette(pal);
        ui->label_cl_lName->setPalette(pal);
        return false;
    }
    return true;
}

bool MainWindow::check_unique_client(){
    QString fname, mname, lname, dob, sin;

    //do not allow for duplicate SIN
    if (ui->lineEdit_cl_SIN->text() != 0) {
        sin = ui->lineEdit_cl_SIN->text();
        QSqlQuery results = dbManager->checkUniqueSIN(sin);
        results.next();
        int numClients = results.value(0).toString().toInt();
        if (numClients > 0) {
            QMessageBox alertBox;
            alertBox.setText("Error: a client with the same SIN number already exists.\nCannot create a new client with this SIN number.");

            QString tmpStyleSheet = MainWindow::styleSheet();
            MainWindow::setStyleSheet("");
            QMessageBox::warning(this, tr("SIN number already exists!"),
                                           tr("Error: a client with the same SIN number already exists.\n"
                                              "Cannot create a new client with this SIN number."),
                                           QMessageBox::Ok);
            MainWindow::setStyleSheet(tmpStyleSheet);
            return false;
        }
    }
    //display dialog with matching name(s)
    if (ui->lineEdit_cl_fName->text() != 0)
        fname = ui->lineEdit_cl_fName->text();

    if (ui->lineEdit_cl_mName->text() != 0)
        mname = ui->lineEdit_cl_mName->text();

    if (ui->lineEdit_cl_lName->text() != 0)
        lname = ui->lineEdit_cl_lName->text();

    if (ui->dateEdit_cl_dob->date().toString(Qt::ISODate) != 0)
        dob = ui->dateEdit_cl_dob->date().toString(Qt::ISODate);

    qDebug() << "fname, mname, lname, dob, sin:" << fname << mname << lname << dob;

    QSqlQuery results = dbManager->checkUniqueClient(fname, mname, lname, dob);

    qDebug() << "num rows affected " << results.numRowsAffected();

    if (results.numRowsAffected() > 0) {
        DuplicateClients *showPossibleClient = new DuplicateClients();
        showPossibleClient->setModal(true);
        connect(showPossibleClient, SIGNAL(selectedUser(QString)), this, SLOT(readSameClientInfo(QString)), Qt::UniqueConnection);
        connect(showPossibleClient, SIGNAL(ignoreWarning()), this, SLOT(ignoreAndRegister()), Qt::UniqueConnection);
        showPossibleClient->displayList(results);
        showPossibleClient->exec();
        qDebug()<<"DuplicateClients";
        qDebug() << "dialog closed";

        return false;
    }
    return true;
}


void MainWindow::readSameClientInfo(QString clientID){
    qDebug()<<"ReadClientInfo " <<clientID;
    registerType = FINDSAMECLIENT;
    curClientID = clientID;
    on_button_cancel_client_register_clicked();
}

void MainWindow::ignoreAndRegister(){
    qDebug()<< "PROCEED REGISTER";
    registerType = NEWCLIENT;
    ignore_duplicate = true;
    qDebug() << "ignore_duplicate set to true";
   // on_button_register_client_clicked();
}


void MainWindow::getCaseWorkerList(){
    QSqlQuery caseWorkers = dbManager->getCaseWorkerList();
    //dbManager->printAll(caseWorkers);
    caseWorkerList.clear();
    while(caseWorkers.next()){
        qDebug()<<"CASEWORKER: " <<caseWorkers.value(0).toString() << caseWorkers.value(1).toString();
        if(!caseWorkerList.contains(caseWorkers.value(0).toString())){
            qDebug()<<"no info of "<<caseWorkers.value(0).toString();
            caseWorkerList.insert(caseWorkers.value(0).toString(), caseWorkers.value(1).toInt());
       }
    }
}

void MainWindow::defaultRegisterOptions(){
    //add caseWorker Name

    if(ui->comboBox_cl_caseWorker->findText("NONE")==-1){
        ui->comboBox_cl_caseWorker->addItem("NONE");
    }

    if(caseWorkerUpdated){
      if(ui->comboBox_cl_caseWorker->count() != caseWorkerList.count()+1){
          qDebug()<<"CASE WORKER LIST UPDATE";
          ui->comboBox_cl_caseWorker->clear();
          ui->comboBox_cl_caseWorker->addItem("NONE");
          QMap<QString, int>::const_iterator it = caseWorkerList.constBegin();
          while(it != caseWorkerList.constEnd()){
            if(ui->comboBox_cl_caseWorker->findText(it.key())== -1){
                ui->comboBox_cl_caseWorker->addItem(it.key());
            }
            ++it;
          }
        }
        caseWorkerUpdated = false;
    }
    if(ui->comboBox_cl_status->findText("Green")==-1){
        ui->comboBox_cl_status->addItem("Green");
        ui->comboBox_cl_status->addItem("Yellow");
        ui->comboBox_cl_status->addItem("Red");
    }

}

void MainWindow::on_button_cancel_client_register_clicked()
{
    clear_client_register_form();
    ui->stackedWidget->setCurrentIndex(CLIENTLOOKUP);
}

/*=============================================================================
 * DELETE CLIENT USING CLIENT ID
 * ==========================================================================*/
void MainWindow::on_button_delete_client_clicked()
{
    //check if the user have permission or not.
    registerType = DELETECLIENT;
    if(currentrole != CASEWORKER && currentrole != ADMIN ){
        statusBar()->showMessage("No permision to delete client! ", 5000);
        ui->button_delete_client->hide();
        return;
    }
    if(ui->lineEdit_cl_fName->text().toLower() == "anonymous" || ui->lineEdit_cl_lName->text().toLower() == "anonymous"){
        statusBar()->showMessage("Cannot Delete anonymous.", 5000);
        ui->button_delete_client->setEnabled(false);
        return;
    }
    deleteClient();

}


void MainWindow::deleteClient(){


    //confirm delete client
    if(!doMessageBox(QString("Do you want delete '" + curClientName + "'?"))){
        return;
    }
    QStringList logFieldList;
    getRegisterLogFields(&logFieldList);
    if(!dbManager->insertClientLog(&logFieldList)){
        statusBar()->showMessage("Fail to delete Client. Please try again. ", 5000);
        return;
    }
    if(!dbManager->deleteClientFromTable("Client", curClientID)){
        statusBar()->showMessage("Fail to delete Client. Please try again. ", 5000);
        return;
    }

    statusBar()->showMessage(QString(curClientName + " is deleted successfully"), 5000);
    curClientID = "";
    curClientName = "";
    registerType = NOREGISTER;
    //maybe need to block update

    //move to main window
    ui->stackedWidget->setCurrentIndex(MAINMENU);
}

/////////////////////////////////////////////////////////////////////////////////


// the add user button
void MainWindow::on_btn_createNewUser_clicked()
{
    // temporary disable stuff
    // obtain username and pw and role from UI
    QString uname = ui->le_userName->text();
    QString pw = ui->le_password->text();
    QString name = ui->lineEdit_EmpName->text();

    caseWorkerUpdated = true;
    if (uname.length() == 0) {
        ui->lbl_editUserWarning->setText("Enter a Username");
        return;
    }

    if (pw.length() == 0) {
        ui->lbl_editUserWarning->setText("Enter a Password");
        return;
    }

    if (name.length() == 0) {
        ui->lbl_editUserWarning->setText("Enter a Name");
        return;
    }

    // first, check to see if the username is taken
    QSqlQuery queryResults = dbManager->findUser(uname);
    int numrows = queryResults.numRowsAffected();

    if (numrows > 0) {
        ui->lbl_editUserWarning->setText("This username is already taken");
        return;
    } else {
        QSqlQuery queryResults = dbManager->addNewEmployee(uname, pw, ui->comboBox->currentText(), name);
        int numrows = queryResults.numRowsAffected();

        if (numrows != 0) {
            ui->lbl_editUserWarning->setText("Employee added");
            QStandardItemModel * model = new QStandardItemModel(0,0);
            model->clear();
            ui->tableWidget_3->clear();
            //ui->tableWidget_3->horizontalHeader()->setStretchLastSection(true);
            ui->tableWidget_3->setColumnCount(4);
            ui->tableWidget_3->setRowCount(0);
            ui->tableWidget_3->setHorizontalHeaderLabels(QStringList() << "Username" << "Password" << "Role" << "Name");

            ui->comboBox->setCurrentIndex(0);
            ui->le_userName->setText("");
            ui->le_password->setText("");
            ui->le_users->setText("");
            ui->lineEdit_EmpName->setText("");

            on_btn_listAllUsers_clicked();
            resizeTableView(ui->tableWidget_3);
        } else {
            ui->lbl_editUserWarning->setText("Something went wrong - please try again");
        }
    }

}


void MainWindow::on_btn_dailyReport_clicked()
{
    ui->swdg_reports->setCurrentIndex(DAILYREPORT);
}

void MainWindow::on_btn_shiftReport_clicked()
{
    ui->swdg_reports->setCurrentIndex(SHIFTREPORT);
}

void MainWindow::on_btn_floatCount_clicked()
{
    ui->swdg_reports->setCurrentIndex(FLOATCOUNT);
}

void MainWindow::on_confirmationFinal_clicked()
{
    curBook = 0;
    curClient = 0;
    trans = 0;
    ui->stackedWidget->setCurrentIndex(MAINMENU);
}



void MainWindow::on_btn_listAllUsers_clicked()
{
    ui->tableWidget_3->setRowCount(0);
    ui->tableWidget_3->clear();
    //ui->tableWidget_3->horizontalHeader()->setStretchLastSection(true);

    QSqlQuery result = dbManager->execQuery("SELECT Username, HASHBYTES('MD5',Password), Role, EmpName FROM Employee");

    int numCols = result.record().count();
    ui->tableWidget_3->setColumnCount(numCols);
    ui->tableWidget_3->setHorizontalHeaderLabels(QStringList() << "Username" << "Password" << "Role" << "Name");
    int x = 0;
    int qt = result.size();
    qDebug() << qt;
    while (result.next()) {
        ui->tableWidget_3->insertRow(x);
        QStringList row;
        row << result.value(0).toString() << result.value(1).toString() << result.value(2).toString() << result.value(3).toString();
        for (int i = 0; i < 4; ++i)
        {
            ui->tableWidget_3->setItem(x, i, new QTableWidgetItem(row.at(i)));
        }
        x++;
    }
    resizeTableView(ui->tableWidget_3);
}

void MainWindow::on_btn_searchUsers_clicked()
{
    QString ename = ui->le_users->text();
    ui->tableWidget_3->setRowCount(0);
    ui->tableWidget_3->clear();
    //ui->tableWidget_3->horizontalHeader()->setStretchLastSection(true);

    QSqlQuery result = dbManager->execQuery("SELECT Username, HASHBYTES('MD5',Password), Role, EmpName FROM Employee WHERE Username LIKE '%"+ ename +"%'");

    int numCols = result.record().count();
    ui->tableWidget_3->setColumnCount(numCols);
    ui->tableWidget_3->setHorizontalHeaderLabels(QStringList() << "Username" << "Password" << "Role" << "Name");
    int x = 0;
    int qt = result.size();
    qDebug() << qt;
    while (result.next()) {
        ui->tableWidget_3->insertRow(x);
        QStringList row;
        row << result.value(0).toString() << result.value(1).toString() << result.value(2).toString() << result.value(3).toString();
        for (int i = 0; i < 4; ++i)
        {
            ui->tableWidget_3->setItem(x, i, new QTableWidgetItem(row.at(i)));
        }
        x++;
    }
    resizeTableView(ui->tableWidget_3);


//    QSqlQuery results = dbManager->execQuery("SELECT Username, Password, Role FROM Employee WHERE Username LIKE '%"+ ename +"%'");
//    QSqlQueryModel *model = new QSqlQueryModel();
//    model->setQuery(results);

////    ui->tableWidget_3->setModel(model);
////    ui->tableWidget_3->horizontalHeader()->model()->setHeaderData(0, Qt::Horizontal, "Username");
////    ui->tableWidget_3->horizontalHeader()->model()->setHeaderData(1, Qt::Horizontal, "Password");
////    ui->tableWidget_3->horizontalHeader()->model()->setHeaderData(2, Qt::Horizontal, "Role");
}





// double clicked employee
void MainWindow::on_tableWidget_3_doubleClicked(const QModelIndex &index)
{
    // populate the fields on the right
//    QString uname = ui->tableWidget_3->model()->data(ui->tableWidget_3->model()->index(index.row(), 0)).toString();
//    QString pw = ui->tableWidget_3->model()->data(ui->tableWidget_3->model()->index(index.row(), 1)).toString();
//    QString role = ui->tableWidget_3->model()->data(ui->tableWidget_3->model()->index(index.row(), 2)).toString();
//    qDebug() << uname;
//    qDebug() << pw;
//    qDebug() << role;

////    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->tableWidget_3->model());
////    int row = index.row();

////     QStandardItemModel* model = ui->tableWidget_3->model();
////    qDebug() << model;
////    QString uname = model->item(row, 0)->text();
////    QString pw = model->item(row, 1)->text();
////    QString role = model->item(row, 2)->text();

//    if (role == "STANDARD") {
//        ui->comboBox->setCurrentIndex(0);
//    } else if (role == "CASE WORKER") {
//        ui->comboBox->setCurrentIndex(1);
//    } else if (role == "ADMIN") {
//        ui->comboBox->setCurrentIndex(2);
//    }

//    ui->le_userName->setText(uname);
//    ui->le_password->setText(pw);
}

void MainWindow::on_pushButton_CaseFiles_clicked()
{
    if (currentrole == STANDARD) {
        return;
    }

    addHistory(CLIENTLOOKUP);
    qDebug()<<"push casefile";
    setSelectedClientInfo();
    ui->stackedWidget->setCurrentIndex(CASEFILE);

    for (auto x: pcp_tables){
        resetPcpTable(x);
    }
    

    //clear old data
    ui->tw_caseFiles->clearContents();
    ui->tw_caseFiles->setRowCount(0);
    ui->le_caseFilter->clear();

    //get client id
    int nRow = ui->tableWidget_search_client->currentRow();
    if (nRow <0)
        return;
    curClientID = ui->tableWidget_search_client->item(nRow, 0)->text();
    QString curFirstName = ui->tableWidget_search_client->item(nRow, 2)->text();
    QString curMiddleName = ui->tableWidget_search_client->item(nRow, 3)->text().size() > 0 ? ui->tableWidget_search_client->item(nRow, 2)->text()+ " " : "";
    QString curLastName = ui->tableWidget_search_client->item(nRow, 1)->text();

    //if dir was saved
//    if (!ui->le_caseDir->text().isEmpty()){
//    QStringList filter = (QStringList() << "*" + ui->tableWidget_search_client->item(nRow, 1)->text() + ", " +
//                          ui->tableWidget_search_client->item(nRow, 2)->text() + "*");

//    qDebug() << "*" + ui->tableWidget_search_client->item(nRow, 1)->text() + ", " +
//                ui->tableWidget_search_client->item(nRow, 2)->text() + "*";
//    ui->le_caseDir->setText(dir.path());


//    populate_tw_caseFiles(filter);
//    }

    //read saved path from db and populate
    QSqlQuery results = dbManager->getCaseFilePath(curClientID);
    results.next();
    dir = results.value(0).toString();
    //check if path exists
    if (dir.exists()) {
        qDebug() << "path exists. using path to populate files";
        ui->le_caseDir->setText(dir.path());
        populate_tw_caseFiles();
    } else {
        qDebug() << "path does not exist. not populating files.";
    }

    ui->lbl_caseClientName->setText(curFirstName + " " + curMiddleName + curLastName + "'s Case Files");
//        QtConcurrent::run(this, retrievePcpData, QString("all"), -1);
    useProgressDialog("Loading PCP tables...", QtConcurrent::run(this, retrievePcpData, QString("all"), -1));
//        retrievePcpData();

    //running notes: move to another function
    QSqlQuery noteResult = dbManager->readNote(curClientID);
    qDebug() << "read note last error: " << noteResult.lastError();
    ui->te_notes->document()->clear();
    noteResult.next();
    ui->te_notes->document()->setPlainText(noteResult.value(0).toString());
}

void MainWindow::resetPcpTable(QTableWidget* table){
    //reset table
    qDebug() << "resetting pcp table ";
    table->clearContents();
    table->setMinimumHeight(73);
    table->setMaximumHeight(1);
    table->setMaximumHeight(16777215);
    table->setRowCount(1);
}

void MainWindow::retrievePcpData(QString type, int tableId) {
    int tableIdx = 0;
    if (type == "all") {
        for (auto x: pcpTypes) {
            dbManager->readPcpThread(curClientID, x, tableIdx++);
        }
    } else {
        dbManager->readPcpThread(curClientID, type, tableId);
    }
}

//void MainWindow::populatePcpTable(QTableWidget *table, QSqlQuery result){
void MainWindow::populatePcpTable(QStringList goal, QStringList strategy, QStringList date, int tableIdx, bool conn){
    if (!conn) {
        qDebug() << "db connection failed";
        return;
    }
    auto table = pcp_tables.at(tableIdx);
    qDebug() << "operating on table " << tableIdx;

//    int numRows = result.numRowsAffected();
    int numRows = goal.count();
    qDebug() << "row count: " << numRows;

    //set number of rows
    qDebug() << "setting number of rows and resizing table height";
    for (int i = 0; i < numRows-1; i++) {
        table->insertRow(0);

        //set height of table
        table->setMinimumHeight(table->minimumHeight() + 35);
    }

    double width = ui->tw_pcpRela->horizontalHeader()->size().width();
    qDebug() << "width of pcp " << width;
    for (auto x: pcp_tables){
        x->resizeRowsToContents();
        x->setColumnWidth(0, width*0.42f);
        x->setColumnWidth(1, width*0.42f);
        x->setColumnWidth(2, width*0.16f);
    }

    //populate table
//    while (result.next()){
//        for (int i = 0; i < 3; i++) {
//            table->setItem(result.value(0).toString().toInt(), i, new QTableWidgetItem(result.value(i+1).toString()));
//        }
//    }
    qDebug() << "populating pcp table " + tableIdx;
    for (int i = 0; i < numRows; i++) {
        table->setItem(i, 0, new QTableWidgetItem(goal[i]));
        table->setItem(i, 1, new QTableWidgetItem(strategy[i]));
        table->setItem(i, 2, new QTableWidgetItem(date[i]));
        qDebug() << i+1 << " rows populated";
    }
}

//CASEFILE WIDGET CHANGE CONTROL
void MainWindow::on_tabw_casefiles_currentChanged(int index)
{
    switch(index)
    {
        case PERSIONACASEPLAN:
            ui->actionExport_to_PDF->setEnabled(true);
            break;
        case RUNNINGNOTE:
            ui->actionExport_to_PDF->setEnabled(true);
            break;
        case BOOKINGHISTORY:
            ui->actionExport_to_PDF->setEnabled(false);
            if(!newHistory)
                break;
            on_pushButton_casefile_book_reload_clicked();
            /*
            initCasefileBookHistoryTable();
            useProgressDialog("Search Transaction...",QtConcurrent::run(this, &searchCasefileBookHistory, curClientID));
            bookingTotal = ui->tableWidget_casefile_booking->rowCount();
            ui->label_casefile_booking_total_num->setText(QString::number(bookingTotal) + " Booking");
            */
            newHistory = false;
            break;

        case TRANSACTIONHISTORY:
            ui->actionExport_to_PDF->setEnabled(false);
            if(!newTrans)
                break;
            on_pushButton_casefile_trans_reload_clicked();
            /*
            initCasefileTransactionTable();
            useProgressDialog("Search Transaction...",QtConcurrent::run(this, &searchCasefileTransaction, curClientID));
            transacTotal = ui->tableWidget_casefile_transaction->rowCount();
            ui->label_casefile_trans_total_num->setText(QString::number(transacTotal) + " Transaction");
            */
            newTrans = false;
            break;

    case CL_RECEIPTS:
        if (!newReceipt) break;
        on_pushButton_casefile_receipt_reload_clicked();
        newReceipt = false;
        break;
    }
}

/*--------------------------------------------------
        CASEFILE BOOKING HISTORY(EUNWON)
----------------------------------------------------*/
//search booking history when click booking history tab
void MainWindow::initCasefileBookHistoryTable(){
    ui->tableWidget_casefile_booking->setRowCount(0);
    ui->tableWidget_casefile_booking->setColumnCount(8);
    ui->tableWidget_casefile_booking->clear();
    ui->tableWidget_casefile_booking->setHorizontalHeaderLabels(QStringList()<<"Reserved Date"<<"Program Code"<<"Start Date"<<"Checkout Date"<<"Cost"<<"Bed Number"<<"FirstBook"<<"Monthly");
}

void MainWindow::searchCasefileBookHistory(QString clientId){
    qDebug()<<"search booking";

    QSqlQuery bookingQuery = dbManager->searchBookList(bookingNum, clientId, CASEFILE);
    displayBookHistory(bookingQuery, ui->tableWidget_casefile_booking);


}

void MainWindow::on_pushButton_casefile_book_reload_clicked()
{
    initCasefileBookHistoryTable();
    ui->tableWidget_casefile_booking->setSortingEnabled(false);
    useProgressDialog("Search Transaction...",QtConcurrent::run(this, &searchCasefileBookHistory, curClientID));
    bookingTotal = ui->tableWidget_casefile_booking->rowCount();
    ui->label_casefile_booking_total_num->setText(QString::number(bookingTotal) + " Bookings");
    ui->tableWidget_casefile_booking->sortByColumn(3, Qt::DescendingOrder);
    ui->tableWidget_casefile_booking->setSortingEnabled(true);
}


/*-----------------------------------------------
        CASEFILE TRANSACTION(EUNWON)
-------------------------------------------------*/
void MainWindow::initCasefileTransactionTable(){
    ui->tableWidget_casefile_transaction->setRowCount(0);

    ui->tableWidget_casefile_transaction->setColumnCount(11);
    ui->tableWidget_casefile_transaction->clear();
    ui->tableWidget_casefile_transaction->setHorizontalHeaderLabels(QStringList()<<"DateTime"<<"TransType"<<"Amount"<<"Payment Type"<<"ChequeNo"<<"MSQ"<<"ChequeDate"
                                                           <<"Deleted"<<"Outstanding"<<"Employee"<<"Notes");

}

//search transaction list when click transaction list
void MainWindow::searchCasefileTransaction(QString clientId){
    qDebug()<<"search transaction STaRt";
    initCasefileTransactionTable();
    QSqlQuery transQuery = dbManager->searchClientTransList(transacNum, clientId, CASEFILE);
    displayTransaction(transQuery, ui->tableWidget_casefile_transaction);
}

//reload client transaction history
void MainWindow::on_pushButton_casefile_trans_reload_clicked()
{
    initCasefileTransactionTable();
    ui->tableWidget_casefile_transaction->setSortingEnabled(false);
    useProgressDialog("Search Transaction...",QtConcurrent::run(this, &searchCasefileTransaction, curClientID));
    transacTotal = ui->tableWidget_casefile_transaction->rowCount();
    ui->label_casefile_trans_total_num->setText(QString::number(transacTotal) + " Transactions");
    ui->tableWidget_casefile_transaction->sortByColumn(0, Qt::DescendingOrder);
    ui->tableWidget_casefile_transaction->setSortingEnabled(true);
}

/*-----------------------------------------------
        CASEFILE RECEIPTS(DENNIS)
-------------------------------------------------*/

//reload client transaction history
void MainWindow::on_pushButton_casefile_receipt_reload_clicked()
{
    initClReceiptTable(ui->tw_cl_receipts);
    QFuture<void> receiptFuture = QtConcurrent::run(this, &searchReceipts, curClientID, ui->tw_cl_receipts);
    useProgressDialog("Loading receipts...", receiptFuture);
    QString receiptTotal = QString::number(ui->tw_cl_receipts->rowCount());
    ui->label_casefile_receipt_total_num->setText(receiptTotal + " Receipts");
}

/*----------------------------------------CASEFILE END------------------------------------------*/


void MainWindow::on_EditRoomsButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(EDITROOM);
    addHistory(ADMINPAGE);

    // set dropdowns
    populate_modRoom_cboxes();

    qDebug() << "pushed page " << ADMINPAGE;
}

// update employee button
void MainWindow::on_pushButton_4_clicked()
{
    // obtain username and pw and role from UI
    QString uname = ui->le_userName->text();
    QString pw = ui->le_password->text();
    QString name = ui->lineEdit_EmpName->text();

    caseWorkerUpdated = true;
    if (uname.length() == 0) {
        ui->lbl_editUserWarning->setText("Enter a Username");
        return;
    }

    if (pw.length() == 0) {
        ui->lbl_editUserWarning->setText("Enter a Password");
        return;
    }

    if (name.length() == 0) {
        ui->lbl_editUserWarning->setText("Enter a Name");
        return;
    }

    // first, check to make sure the username is taken
    QSqlQuery queryResults = dbManager->findUser(uname);
    int numrows1 = queryResults.numRowsAffected();

    if (numrows1 == 1) {
        QSqlQuery queryResults = dbManager->updateEmployee(uname, pw, ui->comboBox->currentText(), name);
        int numrows = queryResults.numRowsAffected();

        if (numrows != 0) {
            ui->lbl_editUserWarning->setText("Employee Updated");
            QStandardItemModel * model = new QStandardItemModel(0,0);
            model->clear();
            ui->tableWidget_3->clear();
            //ui->tableWidget_3->horizontalHeader()->setStretchLastSection(true);
            ui->tableWidget_3->setColumnCount(4);
            ui->tableWidget_3->setRowCount(0);
            ui->tableWidget_3->setHorizontalHeaderLabels(QStringList() << "Username" << "Password" << "Role" << "Name");

            ui->comboBox->setCurrentIndex(0);
            ui->le_userName->setText("");
            ui->le_password->setText("");
            ui->lineEdit_EmpName->setText("");
            ui->le_users->setText("");

            on_btn_listAllUsers_clicked();
            resizeTableView(ui->tableWidget_3);
        } else {
            ui->lbl_editUserWarning->setText("Something went wrong - Please try again");
        }

        return;
    } else {
        ui->lbl_editUserWarning->setText("Employee Not Found");
        return;
    }
}

// Clear button
void MainWindow::on_btn_displayUser_clicked()
{
    QStandardItemModel * model = new QStandardItemModel(0,0);
    model->clear();
    ui->tableWidget_3->clear();
    //ui->tableWidget_3->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_3->setColumnCount(4);
    ui->tableWidget_3->setRowCount(0);
    ui->tableWidget_3->setHorizontalHeaderLabels(QStringList() << "Username" << "Password" << "Role" << "Name");

    ui->comboBox->setCurrentIndex(0);
    ui->le_userName->setText("");
    ui->le_password->setText("");
    ui->lineEdit_EmpName->setText("");
    ui->le_users->setText("");
    resizeTableView(ui->tableWidget_3);
}

void MainWindow::on_tableWidget_3_clicked(const QModelIndex &index)
{
    // populate the fields on the right
    QString uname = ui->tableWidget_3->model()->data(ui->tableWidget_3->model()->index(index.row(), 0)).toString();
    QString pw = ui->tableWidget_3->model()->data(ui->tableWidget_3->model()->index(index.row(), 1)).toString();
    QString role = ui->tableWidget_3->model()->data(ui->tableWidget_3->model()->index(index.row(), 2)).toString();
    QString name = ui->tableWidget_3->model()->data(ui->tableWidget_3->model()->index(index.row(), 3)).toString();
    qDebug() << uname;
    qDebug() << pw;
    qDebug() << role;

//    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->tableWidget_3->model());
//    int row = index.row();

//     QStandardItemModel* model = ui->tableWidget_3->model();
//    qDebug() << model;
//    QString uname = model->item(row, 0)->text();
//    QString pw = model->item(row, 1)->text();
//    QString role = model->item(row, 2)->text();

    if (role == "STANDARD") {
        ui->comboBox->setCurrentIndex(0);
    } else if (role == "CASE WORKER") {
        ui->comboBox->setCurrentIndex(1);
    } else if (role == "ADMIN") {
        ui->comboBox->setCurrentIndex(2);
    }

    ui->le_userName->setText(uname);
    ui->le_password->setText(pw);
    ui->lineEdit_EmpName->setText(name);
}

// delete button
void MainWindow::on_pushButton_6_clicked()
{
    QString uname = ui->le_userName->text();
    QString pw = ui->le_password->text();

    if (uname.length() == 0) {
        ui->lbl_editUserWarning->setText("Please make sure a valid employee is selected");
        return;
    }

    if (pw.length() == 0) {
        ui->lbl_editUserWarning->setText("Please make sure a valid employee is selected");
        return;
    }

    QSqlQuery queryResults = dbManager->findUser(uname);
    int numrows1 = queryResults.numRowsAffected();

    if (numrows1 == 1) {
        QSqlQuery queryResults = dbManager->deleteEmployee(uname, pw, ui->comboBox->currentText());
        int numrows = queryResults.numRowsAffected();

        if (numrows != 0) {
            ui->lbl_editUserWarning->setText("Employee Deleted");
            QStandardItemModel * model = new QStandardItemModel(0,0);
            model->clear();
            ui->tableWidget_3->clear();
            //ui->tableWidget_3->horizontalHeader()->setStretchLastSection(true);
            ui->tableWidget_3->setColumnCount(4);
            ui->tableWidget_3->setRowCount(0);
            ui->tableWidget_3->setHorizontalHeaderLabels(QStringList() << "Username" << "Password" << "Role" << "Name");

            ui->comboBox->setCurrentIndex(0);
            ui->le_userName->setText("");
            ui->le_password->setText("");
            ui->lineEdit_EmpName->setText("");
            ui->le_users->setText("");

            on_btn_listAllUsers_clicked();
            resizeTableView(ui->tableWidget_3);
        } else {
            ui->lbl_editUserWarning->setText("Employee Not Found");
        }

        return;
    } else {
        ui->lbl_editUserWarning->setText("Employee Not Found");
        return;
    }

}

// list all rooms
void MainWindow::on_btn_listAllUsers_3_clicked()
{
    QString ename = ui->le_users_3->text();
    ui->tableWidget_5->setRowCount(0);
    ui->tableWidget_5->clear();
    //ui->tableWidget_5->horizontalHeader()->setStretchLastSection(true);

    QSqlQuery result = dbManager->execQuery("SELECT SpaceCode, cost, Monthly FROM Space ORDER BY SpaceCode");

//    int numCols = result.record().count();
    ui->tableWidget_5->setColumnCount(8);
    ui->tableWidget_5->setHorizontalHeaderLabels(QStringList() << "Space Code" << "Building" << "Floor" << "Room" << "Space #" << "Type" << "Daily Cost" << "Monthly Cost");
    int x = 0;
    int qt = result.size();
    qDebug() << "<" << qt;
    while (result.next()) {
        // break down the spacecode

        QString spacecode = result.value(0).toString();
        if (spacecode == "") {
            break;
        }
        std::string strspacecode = spacecode.toStdString();

        std::vector<std::string> brokenupspacecode = split(strspacecode, '-');
        // parse space code to check building number + floor number + room number + space number
        QString buildingnum = QString::fromStdString(brokenupspacecode[0]);
        QString floornum = QString::fromStdString(brokenupspacecode[1]);
        QString roomnum = QString::fromStdString(brokenupspacecode[2]);
        std::string bednumtype = brokenupspacecode[3];

        qDebug() << "Spacecode type:" << QString::fromStdString(brokenupspacecode[3]);

        // strip the last character
        QString bednumber = QString::fromStdString(bednumtype.substr(0, bednumtype.size()-1));

        qDebug() << bednumber;


        // get the last character to figure out the type
        char typechar = bednumtype[bednumtype.size() - 1];

        qDebug() << typechar;

        QString thetype = "" + typechar;

        qDebug() << thetype;

        ui->tableWidget_5->insertRow(x);
        QStringList row;
        row << spacecode
            << buildingnum
            << floornum
            << roomnum
            << bednumber
            << QChar::fromLatin1(typechar)
            << result.value(1).toString()
            << result.value(2).toString();
        for (int i = 0; i < 8; ++i)
        {
            ui->tableWidget_5->setItem(x, i, new QTableWidgetItem(row.at(i)));
        }
        x++;
    }
    MainWindow::resizeTableView(ui->tableWidget_5);
    MainWindow::addCurrencyToTableWidget(ui->tableWidget_5, 6);
    MainWindow::addCurrencyToTableWidget(ui->tableWidget_5, 7);

}

// list all programs
void MainWindow::on_btn_listAllUsers_2_clicked()
{
    ui->tableWidget_2->setRowCount(0);
    ui->tableWidget_2->clear();
    //ui->tableWidget_2->horizontalHeader()->setStretchLastSection(true);

    QSqlQuery result = dbManager->execQuery("SELECT ProgramCode, Description FROM Program");

    int numCols = result.record().count();
    ui->tableWidget_2->setColumnCount(numCols);
    ui->tableWidget_2->setHorizontalHeaderLabels(QStringList() << "Code" << "Description");
    int x = 0;
    int qt = result.size();
    qDebug() << qt;
    while (result.next()) {
        ui->tableWidget_2->insertRow(x);
        QStringList row;
        row << result.value(0).toString() << result.value(1).toString();
        for (int i = 0; i < 2; ++i)
        {
            ui->tableWidget_2->setItem(x, i, new QTableWidgetItem(row.at(i)));
        }
        x++;
    }
    resizeTableView(ui->tableWidget_2);
}

// search programs by code
void MainWindow::on_btn_searchUsers_2_clicked()
{
    QString ename = ui->le_users_2->text();
    ui->tableWidget_2->setRowCount(0);
    ui->tableWidget_2->clear();
    //ui->tableWidget_2->horizontalHeader()->setStretchLastSection(true);

    QSqlQuery result = dbManager->execQuery("SELECT ProgramCode, Description FROM Program WHERE ProgramCode LIKE '%"+ ename +"%'");

    int numCols = result.record().count();
    ui->tableWidget_2->setColumnCount(numCols);
    ui->tableWidget_2->setHorizontalHeaderLabels(QStringList() << "Code" << "Description");
    int x = 0;
    int qt = result.size();
    qDebug() << qt;
    while (result.next()) {
        ui->tableWidget_2->insertRow(x);
        QStringList row;
        row << result.value(0).toString() << result.value(1).toString();
        for (int i = 0; i < 2; ++i)
        {
            ui->tableWidget_2->setItem(x, i, new QTableWidgetItem(row.at(i)));
        }
        x++;
    }
    resizeTableView(ui->tableWidget_2);
}

// delete program
void MainWindow::on_pushButton_25_clicked()
{
    QString pcode = ui->le_userName_2->text();

    if (pcode.length() == 0) {
        ui->lbl_editUserWarning->setText("Please make sure a valid Program is selected");
        return;
    }

    // remove tag from all beds with this code
    QSqlQuery bednum = dbManager->execQuery("SELECT SpaceCode, ProgramCodes FROM Space WHERE ProgramCodes LIKE '%" + pcode + "%'");
    qDebug() << "SELECT SpaceCode, ProgramCodes FROM Space WHERE ProgramCodes LIKE '%" + pcode + "%'";
    qDebug() << "err" << bednum.lastError();

    int numbeds = 0;
    while (bednum.next()) {
        numbeds++;
    }

    for (int a = 0; a < numbeds; a++) {
        QSqlQuery allbed = dbManager->execQuery("SELECT SpaceCode, ProgramCodes FROM Space WHERE ProgramCodes LIKE '%" + pcode + "%'");
        allbed.next();
        QString thebedcode = allbed.value(0).toString();
        QString selectedtagg = allbed.value(0).toString();

        qDebug() << thebedcode;
        qDebug() << selectedtagg;

        std::string selectedtag = selectedtagg.toStdString();

        std::vector<std::string> tagbreakdown = split(selectedtag, '-');

        // parse space code to check building number + floor number + room number + space number
        QString buildingnum = QString::fromStdString(tagbreakdown[0]);
        QString floornum = QString::fromStdString(tagbreakdown[1]);
        QString roomnum = QString::fromStdString(tagbreakdown[2]);
        std::string bednumtype = tagbreakdown[3];

        // strip the last character
        QString bednumber = QString::fromStdString(bednumtype.substr(0, bednumtype.size()-1));

        // get space id
        QSqlQuery singlebedquery = dbManager->searchSingleBed(buildingnum, floornum, roomnum, bednumber);
        singlebedquery.next();
        QString spaceid = singlebedquery.value(3).toString();

        // get curr tag
        QString currenttag = singlebedquery.value(8).toString();

        qDebug() << currenttag;

        // remove tag
        QString newtag = "";
        std::string curtagtogether = currenttag.toStdString();
        std::vector<std::string> curtagbreakdown = split(curtagtogether, ',');
        for (std::string t : curtagbreakdown) {
            if (QString::fromStdString(t) != pcode) {
                newtag += QString::fromStdString(t);
                newtag += ",";
            }
        }
        std::string tempstr = newtag.toStdString();
        tempstr.pop_back();
        newtag = QString::fromStdString(tempstr);

        // update tag value
        dbManager->updateSpaceProgram(spaceid, newtag);
    }

    QSqlQuery queryResults = dbManager->execQuery("DELETE FROM Program WHERE ProgramCode='" + pcode + "'");
    int numrows = queryResults.numRowsAffected();

    if (numrows > 0) {
        ui->lbl_editProgWarning->setText("Program Deleted");
        QStandardItemModel * model = new QStandardItemModel(0,0);
        model->clear();
        ui->tableWidget_2->clear();
        ui->tableWidget_2->horizontalHeader()->setStretchLastSection(true);
        ui->tableWidget_2->setColumnCount(2);
        ui->tableWidget_2->setRowCount(0);
        ui->tableWidget_2->setHorizontalHeaderLabels(QStringList() << "Program Code" << "Description");

        ui->comboBox->setCurrentIndex(0);
        ui->le_userName->setText("");
        ui->le_password->setText("");
        ui->le_users->setText("");
    } else {
        ui->lbl_editProgWarning->setText("Program Not Deleted - make sure valid program is selected");
    }
    //HANK

    return;
}

// program clicked + selected
void MainWindow::on_tableWidget_2_clicked(const QModelIndex &index)
{
    if (!resettingfromcode) {
        if (index == lastprogramclicked) {
            return;
        }
    }
        ui->availablebedslist->setHorizontalHeaderLabels(QStringList() << "Space Code");
        ui->assignedbedslist->setHorizontalHeaderLabels(QStringList() << "Space Code");
        ui->lbl_editProgWarning->setText("Please hold while we set your spaces");
        qApp->processEvents();

        ui->availablebedslist->clear();
        ui->availablebedslist->setRowCount(0);

        ui->assignedbedslist->clear();
        ui->assignedbedslist->setRowCount(0);

        ui->availablebedslist->setHorizontalHeaderLabels(QStringList() << "Space Code");
        ui->assignedbedslist->setHorizontalHeaderLabels(QStringList() << "Space Code");

        // populate the fields on the right
        QString pcode = ui->tableWidget_2->model()->data(ui->tableWidget_2->model()->index(index.row(), 0)).toString();
        QString description = ui->tableWidget_2->model()->data(ui->tableWidget_2->model()->index(index.row(), 1)).toString();

        ui->le_userName_2->setText(pcode);
        ui->textEdit->setText(description);

        // populate the beds list
        QSqlQuery availSpaces = dbManager->getAvailableBeds(pcode);
        int numrowsavail = availSpaces.numRowsAffected();
        if (numrowsavail > 0) {
//            int numCols = availSpaces.record().count();
            ui->availablebedslist->setColumnCount(1);
            int x = 0;
            int qt = availSpaces.size();
            qDebug() << qt;
            while (availSpaces.next()) {
                ui->availablebedslist->insertRow(x);
                QStringList row;
                QString buildingNo = availSpaces.value(0).toString();
                QString floorNo = availSpaces.value(1).toString();
                QString roomNo = availSpaces.value(2).toString();
                QString spaceNo = availSpaces.value(4).toString();
                QString type = availSpaces.value(5).toString();

                //1-319-3
                QString roomCode = buildingNo + "-" + floorNo + "-" + roomNo + "-" + spaceNo + type[0];

                ui->availablebedslist->setItem(x, 0, new QTableWidgetItem(roomCode));
                x++;
            }
        }
        ui->availablebedslist->setHorizontalHeaderLabels(QStringList() << "Space Code");
        ui->assignedbedslist->setHorizontalHeaderLabels(QStringList() << "Space Code");
        qApp->processEvents();

    //    QStandardItemModel* availmodel = new QStandardItemModel();

    //    while (availSpaces.next()) {
    //        QString buildingNo = availSpaces.value(0).toString();
    //        QString floorNo = availSpaces.value(1).toString();
    //        QString roomNo = availSpaces.value(2).toString();
    //        QString spaceNo = availSpaces.value(4).toString();
    //        QString type = availSpaces.value(5).toString();

    //        //1-319-3
    //        QString roomCode = buildingNo + "-" + floorNo + "-" + roomNo + "-" + spaceNo + type[0];

    //        // QStandardItem* Items = new QStandardItem(availSpaces.value(1).toString());
    //        QStandardItem* Items = new QStandardItem(roomCode);
    //        availmodel->appendRow(Items);
    //    }

    //    ui->availablebedslist->setModel(availmodel);

          QSqlQuery assignedspaces = dbManager->getAssignedBeds(pcode);
          int numrowsassigned = assignedspaces.numRowsAffected();
          if (numrowsassigned > 0) {
//              int numCols = assignedspaces.record().count();
              ui->assignedbedslist->setColumnCount(1);
              int x = 0;
              int qt = assignedspaces.size();
              qDebug() << qt;
              while (assignedspaces.next()) {
                  ui->assignedbedslist->insertRow(x);
                  QStringList row;
                  QString buildingNo = assignedspaces.value(0).toString();
                  QString floorNo = assignedspaces.value(1).toString();
                  QString roomNo = assignedspaces.value(2).toString();
                  QString spaceNo = assignedspaces.value(4).toString();
                  QString type = assignedspaces.value(5).toString();

                  //1-319-3
                  QString roomCode = buildingNo + "-" + floorNo + "-" + roomNo + "-" + spaceNo + type[0];

                  ui->assignedbedslist->setItem(x, 0, new QTableWidgetItem(roomCode));
                  x++;
              }
          }
          ui->availablebedslist->setHorizontalHeaderLabels(QStringList() << "Space Code");
          ui->assignedbedslist->setHorizontalHeaderLabels(QStringList() << "Space Code");
          ui->lbl_editProgWarning->setText("");

    //    int numrowsassigned = assignedspaces.numRowsAffected();

    //    QStandardItemModel* assignedmodel = new QStandardItemModel();

    //    while (assignedspaces.next()) {
    //        QString buildingNo = assignedspaces.value(0).toString();
    //        QString floorNo = assignedspaces.value(1).toString();
    //        QString roomNo = assignedspaces.value(2).toString();
    //        QString spaceNo = assignedspaces.value(4).toString();
    //        QString type = assignedspaces.value(5).toString();

    //        //1-319-3
    //        QString roomCode = buildingNo + "-" + floorNo + "-" + roomNo + "-" + spaceNo + type[0];

    //        // QStandardItem* Items = new QStandardItem(availSpaces.value(1).toString());
    //        QStandardItem* Items = new QStandardItem(roomCode);
    //        assignedmodel->appendRow(Items);
    //    }

        // ui->assignedbedslist->setModel(assignedmodel);
          lastprogramclicked = index;


}

// set case files directory
void MainWindow::on_pushButton_3_clicked()
{
//    const QString DEFAULT_DIR_KEY("default_dir");
//    QSettings mruDir;
    QString tempDir = QFileDialog::getExistingDirectory(
                    this,
                    tr("Select Directory")
//                    mruDir.value(DEFAULT_DIR_KEY).toString()
                );
    if (!tempDir.isEmpty()) {
        dir = tempDir;
        //try 3 times to save path to db
        for (int i = 0; i < 3; i++) {
            if (dbManager->setCaseFilePath(curClientID, tempDir)) break;
        }
//        mruDir.setValue(DEFAULT_DIR_KEY, tempDir);
        ui->le_caseDir->setText(dir.path());
        populate_tw_caseFiles();
    }
    connect(ui->tw_caseFiles, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(on_tw_caseFiles_cellDoubleClicked(int,int)), Qt::UniqueConnection);
}

// open case file in external reader
void MainWindow::on_tw_caseFiles_cellDoubleClicked(int row, int column)
{
    qDebug() << QUrl::fromLocalFile(dir.absoluteFilePath(ui->tw_caseFiles->item(row, column)->text())) << "at" << row << " " << column;
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absoluteFilePath(ui->tw_caseFiles->item(row, column)->text())));
}

// filter file names
void MainWindow::on_btn_caseFilter_clicked()
{
    qDebug() << "filter button clicked, filter with" << ui->le_caseFilter->text();
    QStringList filter = (QStringList() << "*"+(ui->le_caseFilter->text())+"*");
    populate_tw_caseFiles(filter);
}

void MainWindow::populate_tw_caseFiles(QStringList filter){
    int i = 0;
    dir.setNameFilters(filter);
    for (auto x : dir.entryList(QDir::Files)) {
        qDebug() << x;
        ui->tw_caseFiles->setRowCount(i+1);
        ui->tw_caseFiles->setItem(i++,0, new QTableWidgetItem(x));
//        ui->tw_caseFiles->resizeColumnsToContents();
        ui->tw_caseFiles->resizeRowsToContents();
    }
    if (i > 0) {
        ui->btn_caseFilter->setEnabled(true);
    }
}

// create new program button
void MainWindow::on_btn_createNewUser_2_clicked()
{
    QString pcode = ui->le_userName_2->text();
    QString pdesc = ui->textEdit->toPlainText();

    if (pcode.length() == 0) {
        ui->lbl_editProgWarning->setText("Please Enter a Program Code");
        return;
    }

    if (pdesc.length() == 0) {
        ui->lbl_editProgWarning->setText("Please Enter a Program Description");
        return;
    }

    QSqlQuery queryResults = dbManager->execQuery("SELECT * FROM Program WHERE ProgramCode='" + pcode + "'");
    int numrows = queryResults.numRowsAffected();

    if (numrows == 1) {
        ui->lbl_editProgWarning->setText("Program code in use");
        return;
    } else {
        QSqlQuery qr = dbManager->AddProgram(pcode, pdesc);
        if (qr.numRowsAffected() == 1) {
            ui->lbl_editProgWarning->setText("Program Added");
            QStandardItemModel * model = new QStandardItemModel(0,0);
            model->clear();
            ui->tableWidget_2->clear();
            ui->tableWidget_2->horizontalHeader()->setStretchLastSection(true);
            ui->tableWidget_2->setColumnCount(2);
            ui->tableWidget_2->setRowCount(0);
            ui->tableWidget_2->setHorizontalHeaderLabels(QStringList() << "Program Code" << "Description");

            ui->comboBox->setCurrentIndex(0);
            ui->le_userName->setText("");
            ui->le_password->setText("");
            ui->le_users->setText("");
        } else {
            ui->lbl_editProgWarning->setText("Program not added - please try again.");
        }
    }
}

// update program
void MainWindow::on_pushButton_24_clicked()
{
    QString pcode = ui->le_userName_2->text();
    QString pdesc = ui->textEdit->toPlainText();

    if (pcode.length() == 0) {
        ui->lbl_editProgWarning->setText("Please Enter a Program Code");
        return;
    }

    if (pdesc.length() == 0) {
        ui->lbl_editProgWarning->setText("Please Enter a Program Description");
        return;
    }

    QSqlQuery queryResults = dbManager->execQuery("SELECT * FROM Program WHERE ProgramCode='" + pcode + "'");
    int numrows = queryResults.numRowsAffected();

    if (numrows != 1) {
        ui->lbl_editProgWarning->setText("Enter a valid program code to update");
        return;
    } else {
        QSqlQuery qr = dbManager->updateProgram(pcode, pdesc);
        if (qr.numRowsAffected() == 1) {
            ui->lbl_editProgWarning->setText("Program Updated");
            QStandardItemModel * model = new QStandardItemModel(0,0);
            model->clear();
            ui->tableWidget_2->clear();
            ui->tableWidget_2->horizontalHeader()->setStretchLastSection(true);
            ui->tableWidget_2->setColumnCount(2);
            ui->tableWidget_2->setRowCount(0);
            ui->tableWidget_2->setHorizontalHeaderLabels(QStringList() << "Program Code" << "Description");

            ui->comboBox->setCurrentIndex(0);
            ui->le_userName->setText("");
            ui->le_password->setText("");
            ui->le_users->setText("");
        } else {
            ui->lbl_editProgWarning->setText("Program not updated - please try again.");
        }
    }
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    Q_UNUSED(event);
    double width = ui->tw_pcpRela->horizontalHeader()->size().width();
    for (auto x: pcp_tables){
        x->resizeRowsToContents();
        x->setColumnWidth(0, width*0.42f);
        x->setColumnWidth(1, width*0.42f);
        x->setColumnWidth(2, width*0.16f);
    }
}

//void MainWindow::on_tw_pcpRela_itemChanged(QTableWidgetItem *item)
//{
//    for (auto x: pcp_tables){
//        x->resizeRowsToContents();
//    }
//}

void MainWindow::initPcp(){
    pcp_tables.push_back(ui->tw_pcpRela);
    pcp_tables.push_back(ui->tw_pcpEdu);
    pcp_tables.push_back(ui->tw_pcpSub);
    pcp_tables.push_back(ui->tw_pcpAcco);
    pcp_tables.push_back(ui->tw_pcpLife);
    pcp_tables.push_back(ui->tw_pcpMent);
    pcp_tables.push_back(ui->tw_pcpPhy);
    pcp_tables.push_back(ui->tw_pcpLeg);
    pcp_tables.push_back(ui->tw_pcpAct);
    pcp_tables.push_back(ui->tw_pcpTrad);
    pcp_tables.push_back(ui->tw_pcpOther);
    pcp_tables.push_back(ui->tw_pcpPpl);

    pcpTypes = {
                    "relationship",
                    "educationEmployment",
                    "substanceUse",
                    "accomodationsPlanning",
                    "lifeSkills",
                    "mentalHealth",
                    "physicalHealth",
                    "legalInvolvement",
                    "activities",
                    "traditions",
                    "other",
                    "people"
                };

}

void MainWindow::on_btn_pcpRela_clicked()
{
    ui->tw_pcpRela->insertRow(0);
    ui->tw_pcpRela->setMinimumHeight(ui->tw_pcpRela->minimumHeight()+35);
}

void MainWindow::on_btn_pcpEdu_clicked()
{
    ui->tw_pcpEdu->insertRow(0);
    ui->tw_pcpEdu->setMinimumHeight(ui->tw_pcpEdu->minimumHeight()+35);
}

void MainWindow::on_btn_pcpSub_clicked()
{
    ui->tw_pcpSub->insertRow(0);
    ui->tw_pcpSub->setMinimumHeight(ui->tw_pcpSub->minimumHeight()+35);
}

void MainWindow::on_btn_pcpAcc_clicked()
{
    ui->tw_pcpAcco->insertRow(0);
    ui->tw_pcpAcco->setMinimumHeight(ui->tw_pcpAcco->minimumHeight()+35);
}

void MainWindow::on_btn_pcpLife_clicked()
{
    ui->tw_pcpLife->insertRow(0);
    ui->tw_pcpLife->setMinimumHeight(ui->tw_pcpLife->minimumHeight()+35);
}

void MainWindow::on_btn_pcpMent_clicked()
{
    ui->tw_pcpMent->insertRow(0);
    ui->tw_pcpMent->setMinimumHeight(ui->tw_pcpMent->minimumHeight()+35);
}

void MainWindow::on_btn_pcpPhy_clicked()
{
    ui->tw_pcpPhy->insertRow(0);
    ui->tw_pcpPhy->setMinimumHeight(ui->tw_pcpPhy->minimumHeight()+35);
}

void MainWindow::on_btn_pcpLeg_clicked()
{
    ui->tw_pcpLeg->insertRow(0);
    ui->tw_pcpLeg->setMinimumHeight(ui->tw_pcpLeg->minimumHeight()+35);
}

void MainWindow::on_btn_pcpAct_clicked()
{
    ui->tw_pcpAct->insertRow(0);
    ui->tw_pcpAct->setMinimumHeight(ui->tw_pcpAct->minimumHeight()+35);
}

void MainWindow::on_btn_pcpTrad_clicked()
{
    ui->tw_pcpTrad->insertRow(0);
    ui->tw_pcpTrad->setMinimumHeight(ui->tw_pcpTrad->minimumHeight()+35);
}

void MainWindow::on_btn_pcpOther_clicked()
{
    ui->tw_pcpOther->insertRow(0);
    ui->tw_pcpOther->setMinimumHeight(ui->tw_pcpOther->minimumHeight()+35);
}

void MainWindow::on_btn_pcpKey_clicked()
{
    ui->tw_pcpPpl->insertRow(0);
    ui->tw_pcpPpl->setMinimumHeight(ui->tw_pcpPpl->minimumHeight()+35);
}

void MainWindow::on_addbedtoprogram_clicked()
{
    // add program tag to bed
    QString pcode = ui->le_userName_2->text();

    if (pcode== "") {
        return;
    }

    if ((ui->availablebedslist->selectedItems().size()) == 0) {
        return;
    }

    // QModelIndexList qil = ui->availablebedslist->selectedIndexes();

    // get selected bed
    qDebug() << ui->availablebedslist->currentItem()->text();
    std::string selectedtag = ui->availablebedslist->currentItem()->text().toStdString();

    std::vector<std::string> tagbreakdown = split(selectedtag, '-');

    // parse space code to check building number + floor number + room number + space number
    QString buildingnum = QString::fromStdString(tagbreakdown[0]);
    QString floornum = QString::fromStdString(tagbreakdown[1]);
    QString roomnum = QString::fromStdString(tagbreakdown[2]);
    std::string bednumtype = tagbreakdown[3];
    // strip the last character
    QString bednumber = QString::fromStdString(bednumtype.substr(0, bednumtype.size()-1));

    // get space id
    QSqlQuery singlebedquery = dbManager->searchSingleBed(buildingnum, floornum, roomnum, bednumber);
    singlebedquery.next();
    QString spaceid = singlebedquery.value(3).toString();

    // get curr tag
    QString currenttag = singlebedquery.value(8).toString();
    if (currenttag != "") {
        // append to tag
        currenttag += ",";
    }
    currenttag += pcode;

    qDebug() << currenttag;

    // update tag value
    dbManager->updateSpaceProgram(spaceid, currenttag);

    resettingfromcode = true;

    on_tableWidget_2_clicked(lastprogramclicked);

    resettingfromcode = false;

    // ui->availablebedslist->clear();
    // ui->availablebedslist->setRowCount(0);
    // ui->assignedbedslist->clear();
    // ui->assignedbedslist->setRowCount(0);
    //ui->availablebedslist->setHorizontalHeaderLabels(QStringList() << "Bed Code");
    //ui ->assignedbedslist->setHorizontalHeaderLabels(QStringList() << "Bed Code");
    ui->lbl_editProgWarning->setText("Bed Added to Program");

//    const auto selectedIdxs = ui->availablebedslist->selectedIndexes();
//    if (!selectedIdxs.isEmpty()) {
//        const QVariant var = ui->availablebedslist->model()->data(selectedIdxs.first());
//        // next you need to convert your `var` from `QVariant` to something that you show from your data with default (`Qt::DisplayRole`) role, usually it is a `QString`:
//        const QString selectedItemString = var.toString();

//        // or you also may do this by using `QStandardItemModel::itemFromIndex()` method:
//        const QStandardItem* selectedItem = dynamic_cast<QStandardItemModel*>(model())->itemFromIndex(selectedIdxs.first());
//        // use your `QStandardItem`
//    }

    // populate the beds list
//    QSqlQuery availSpaces = dbManager->getAvailableBeds(pcode);
//    int numrowsavail = availSpaces.numRowsAffected();

//    QStandardItemModel* availmodel = new QStandardItemModel();

//    while (availSpaces.next()) {
//        QString buildingNo = availSpaces.value(0).toString();
//        QString floorNo = availSpaces.value(1).toString();
//        QString roomNo = availSpaces.value(2).toString();
//        QString spaceNo = availSpaces.value(4).toString();
//        QString type = availSpaces.value(5).toString();

//        //1-319-3
//        QString roomCode = buildingNo + "-" + floorNo + "-" + roomNo + "-" + spaceNo + type[0];

//        // QStandardItem* Items = new QStandardItem(availSpaces.value(1).toString());
//        QStandardItem* Items = new QStandardItem(roomCode);
//        availmodel->appendRow(Items);
//    }

//    ui->availablebedslist->setModel(availmodel);


}

void MainWindow::on_removebedfromprogram_clicked()
{
    // remove program tag from bed
    QString pcode = ui->le_userName_2->text();

    if (pcode== "") {
        return;
    }

    if ((ui->assignedbedslist->selectedItems().size()) == 0) {
        return;
    }

    // get selected bed
    qDebug() << ui->assignedbedslist->currentItem()->text();
    std::string selectedtag = ui->assignedbedslist->currentItem()->text().toStdString();

    std::vector<std::string> tagbreakdown = split(selectedtag, '-');

    // parse space code to check building number + floor number + room number + space number
    QString buildingnum = QString::fromStdString(tagbreakdown[0]);
    QString floornum = QString::fromStdString(tagbreakdown[1]);
    QString roomnum = QString::fromStdString(tagbreakdown[2]);
    std::string bednumtype = tagbreakdown[3];
    // strip the last character
    QString bednumber = QString::fromStdString(bednumtype.substr(0, bednumtype.size()-1));

    // get space id
    QSqlQuery singlebedquery = dbManager->searchSingleBed(buildingnum, floornum, roomnum, bednumber);
    singlebedquery.next();
    QString spaceid = singlebedquery.value(3).toString();

    // get curr tag
    QString currenttag = singlebedquery.value(8).toString();

    // remove tag
    QString newtag = "";
    std::string curtagtogether = currenttag.toStdString();
    std::vector<std::string> curtagbreakdown = split(curtagtogether, ',');
    for (std::string t : curtagbreakdown) {
        if (QString::fromStdString(t) != pcode) {
            newtag += QString::fromStdString(t);
            newtag += ",";
        }
    }
    std::string tempstr = newtag.toStdString();
    if (tempstr != "") {
        tempstr.pop_back();
    }
    newtag = QString::fromStdString(tempstr);

    qDebug() << newtag;

    // update tag value
    dbManager->updateSpaceProgram(spaceid, newtag);
//    ui->availablebedslist->clear();
//    ui->availablebedslist->setRowCount(0);
//    ui->assignedbedslist->clear();
//    ui->assignedbedslist->setRowCount(0);
//    ui->availablebedslist->setHorizontalHeaderLabels(QStringList() << "Bed Code");
//    ui->assignedbedslist->setHorizontalHeaderLabels(QStringList() << "Bed Code");
    resettingfromcode = true;
    on_tableWidget_2_clicked(lastprogramclicked);
    resettingfromcode = false;
    ui->lbl_editProgWarning->setText("Bed Removed from Program");
}

void MainWindow::on_availablebedslist_clicked(const QModelIndex &index)
{
    // clicked available bed
//    availableBedIndex = index;
//    QStringList list;
//    QStandardItemModel model = (QStandardItemModel) ui->availablebedslist->model();
//    foreach(const QModelIndex &index, ui->availablebedslist->selectionModel()->selectedIndexes()) {
//        list.append(model->itemFromIndex(index)->text());
//    }
}

void MainWindow::on_assignedbedslist_clicked(const QModelIndex &index)
{
    // clicked assigned bed
    assignedBedIndex = index;
}

void MainWindow::on_btn_monthlyReport_clicked()
{
    ui->swdg_reports->setCurrentIndex(MONTHLYREPORT);
}


void MainWindow::on_btn_restrictedList_clicked()
{
    ui->swdg_reports->setCurrentIndex(RESTRICTIONS);
}


/*==============================================================================
REPORTS
==============================================================================*/
void MainWindow::setupReportsScreen()
{
    // Daily Report Screen
    ui->lbl_dailyReportDateVal->setText(QDate::currentDate().toString(Qt::ISODate));
    ui->dailyReport_dateEdit->setDate(QDate::currentDate());
    checkoutReport = new Report(this, ui->checkout_tableView, CHECKOUT_REPORT);
    vacancyReport = new Report(this, ui->vacancy_tableView, VACANCY_REPORT);
    lunchReport = new Report(this, ui->lunch_tableView, LUNCH_REPORT);
    wakeupReport = new Report(this, ui->wakeup_tableView, WAKEUP_REPORT);

    // Shift Report Screen
    ui->lbl_shiftReportDateVal->setText(QDate::currentDate().toString(Qt::ISODate));
    ui->lbl_shiftReportShiftVal->setText(QString::number(currentshiftid));
    ui->shiftReport_dateEdit->setDate(QDate::currentDate());
    ui->shiftReport_spinBox->setValue(currentshiftid);

    bookingReport = new Report(this, ui->booking_tableView, BOOKING_REPORT);
    transactionReport = new Report(this, ui->transaction_tableView, TRANSACTION_REPORT);
    clientLogReport = new Report(this, ui->clientLog_tableView, CLIENT_REPORT);
    otherReport = new Report(this, ui->other_tableView, OTHER_REPORT);
    yellowReport = new Report(this, ui->yellowRestriction_tableView, YELLOW_REPORT);
    redReport = new Report(this, ui->redRestriction_tableView, RED_REPORT);

    ui->saveOther_btn->setEnabled(false);

    // Float Report Screen
    ui->cashFloatDate_lbl->setText(QDate::currentDate().toString(Qt::ISODate));
    ui->cashFloatShift_lbl->setText(QString::number(currentshiftid));
    ui->cashFloat_dateEdit->setDate(QDate::currentDate());
    ui->cashFloat_spinBox->setValue(currentshiftid);

    // Monthly Report Screen
    QStringList monthList;
    monthList << "Jan" << "Feb" << "Mar" << "Apr" << "May" << "Jun" << "Jul" << "Aug" << "Sep" << "Oct" << "Nov" << "Dec";
    ui->month_comboBox->addItems(monthList);

    QStringList yearList;
    for (int i = 2016; i <= QDate::currentDate().year(); i++)
    {
        yearList << QString::number(i);
    }
    ui->year_comboBox->addItems(yearList);

    ui->month_comboBox->setCurrentIndex(QDate::currentDate().month() - 1);
    ui->year_comboBox->setCurrentIndex(yearList.size() - 1);

    connect(&(checkoutReport->model), SIGNAL(modelDataUpdated(int, int)), this,
        SLOT(on_modelDataUpdated(int, int)),Qt::QueuedConnection);
    connect(&(vacancyReport->model), SIGNAL(modelDataUpdated(int, int)), this,
        SLOT(on_modelDataUpdated(int, int)),Qt::QueuedConnection);
    connect(&(lunchReport->model), SIGNAL(modelDataUpdated(int, int)), this,
        SLOT(on_modelDataUpdated(int, int)),Qt::QueuedConnection);
    connect(&(wakeupReport->model), SIGNAL(modelDataUpdated(int, int)), this,
        SLOT(on_modelDataUpdated(int, int)),Qt::QueuedConnection);
    connect(&(bookingReport->model), SIGNAL(modelDataUpdated(int, int)), this,
        SLOT(on_modelDataUpdated(int, int)),Qt::QueuedConnection);
    connect(&(transactionReport->model), SIGNAL(modelDataUpdated(int, int)), this,
        SLOT(on_modelDataUpdated(int, int)),Qt::QueuedConnection);
    connect(&(clientLogReport->model), SIGNAL(modelDataUpdated(int, int)), this,
        SLOT(on_modelDataUpdated(int, int)),Qt::QueuedConnection);
    connect(&(otherReport->model), SIGNAL(modelDataUpdated(int, int)), this,
        SLOT(on_modelDataUpdated(int, int)),Qt::QueuedConnection);
}

void MainWindow::on_modelDataUpdated(int reportType, int cols)
{
    switch(reportType)
    {
        case CHECKOUT_REPORT:
            resizeTableView(ui->checkout_tableView);
            break;
        case VACANCY_REPORT:
            resizeTableView(ui->vacancy_tableView);
            break;
        case LUNCH_REPORT:
            resizeTableView(ui->lunch_tableView);
            break;
        case WAKEUP_REPORT:
            resizeTableView(ui->wakeup_tableView);
            break;
        case BOOKING_REPORT:
            resizeTableView(ui->booking_tableView);
            break;
        case TRANSACTION_REPORT:
            resizeTableView(ui->transaction_tableView);
            break;
        case CLIENT_REPORT:
            resizeTableView(ui->clientLog_tableView);
            break;
        case OTHER_REPORT:
            resizeTableView(ui->other_tableView);
    }
}

void MainWindow::resizeTableView(QTableView* tableView)
{
    tableView->setVisible(false);
    tableView->resizeColumnsToContents();

    int cols = tableView->horizontalHeader()->count();
    double width = tableView->horizontalHeader()->size().width();
    double currentWidth = 0;
    int hiddenColCount = 0;
    for (int i = 0; i < cols; ++i)
    {
        double colWidth = tableView->columnWidth(i);
        if (colWidth == 0)
        {
            hiddenColCount++;
        }
        currentWidth +=  colWidth;
    }
    double diff = width - currentWidth;
    if (diff > 0)
    {
        double sizeIncrease = diff / (cols - hiddenColCount);

        for (int i = 0; i < cols; ++i)
        {
            if (diff > currentWidth * 1.5f)
            {
                sizeIncrease = tableView->columnWidth(i) * 0.75f;
            }
            if (tableView->columnWidth(i) != 0)
            {
                tableView->setColumnWidth(i, tableView->columnWidth(i) + sizeIncrease);
            }
        }
    }

    tableView->setVisible(true);
}

void MainWindow::updateDailyReportTables(QDate date)
{
    // useProgressDialog("Processing reports...",
    //     QtConcurrent::run(checkoutReport, &checkoutReport->updateModelThread, date));
    checkoutReport->updateModel(date);
    vacancyReport->updateModel(date);
    lunchReport->updateModel(date);
    wakeupReport->updateModel(date);

    ui->lbl_dailyReportDateVal->setText(date.toString(Qt::ISODate));
    ui->dailyReport_dateEdit->setDate(date);
}

void MainWindow::updateRestrictionTables()
{
    useProgressDialog("Processing reports...",
        QtConcurrent::run(yellowReport, &yellowReport->updateModelThread));
    //yellowReport->updateModel();
    redReport->updateModel();

    ui->restrictionLastUpdated_lbl->setText(QString(QDate::currentDate().toString(Qt::ISODate)
        + " at " + QTime::currentTime().toString("h:mmAP")));
}

void MainWindow::updateShiftReportTables(QDate date, int shiftNo)
{
    // useProgressDialog("Processing reports...",
    //     QtConcurrent::run(bookingReport, &bookingReport->updateModelThread, date, shiftNo));
    bookingReport->updateModel(date, shiftNo);
    transactionReport->updateModel(date, shiftNo);
    clientLogReport->updateModel(date, shiftNo);
    otherReport->updateModel(date, shiftNo);

    ui->lbl_shiftReportDateVal->setText(date.toString(Qt::ISODate));
    ui->lbl_shiftReportShiftVal->setText(QString::number(shiftNo));
    ui->shiftReport_dateEdit->setDate(date);
    ui->shiftReport_spinBox->setValue(shiftNo);
}

void MainWindow::on_dailyReportGo_btn_clicked()
{
    QDate date = ui->dailyReport_dateEdit->date();

    MainWindow::updateDailyReportTables(date);
    MainWindow::getDailyReportStats(date);
}

void MainWindow::on_dailyReportCurrent_btn_clicked()
{
    ui->dailyReport_dateEdit->setDate(QDate::currentDate());
    MainWindow::on_dailyReportGo_btn_clicked();
}

void MainWindow::on_shiftReportGo_btn_clicked()
{
    QDate date = ui->shiftReport_dateEdit->date();
    int shiftNo = ui->shiftReport_spinBox->value();

    MainWindow::updateShiftReportTables(date, shiftNo);
    MainWindow::getShiftReportStats(date, shiftNo);
}

void MainWindow::on_shiftReportCurrent_btn_clicked()
{
    ui->shiftReport_dateEdit->setDate(QDate::currentDate());
    ui->shiftReport_spinBox->setValue(currentshiftid);
    MainWindow::on_shiftReportGo_btn_clicked();
}

void MainWindow::on_other_lineEdit_textEdited(const QString &text)
{
    if (text.trimmed().isEmpty())
    {
        ui->saveOther_btn->setEnabled(false);
    }
    else
    {
        ui->saveOther_btn->setEnabled(true);
    }
}

void MainWindow::on_saveOther_btn_clicked()
{
    QString logText = ui->other_lineEdit->text();
    if (dbManager->insertOtherLog(usernameLoggedIn, currentshiftid, logText))
    {
        statusBar()->showMessage(tr("New log saved"), 5000);
        ui->other_lineEdit->clear();
    }
    else
    {
        statusBar()->showMessage(tr("Could not save new log"), 5000);
    }
    QDate date = ui->shiftReport_dateEdit->date();
    int shiftNo = ui->shiftReport_spinBox->value();
   useProgressDialog("Processing reports...",
       QtConcurrent::run(otherReport, &bookingReport->updateModelThread, date, shiftNo));

}

void MainWindow::getDailyReportStats(QDate date)
{
    useProgressDialog("Processing reports...",
        QtConcurrent::run(dbManager, &DatabaseManager::getDailyReportStatsThread, date));
}

void MainWindow::getShiftReportStats(QDate date, int shiftNo)
{
    useProgressDialog("Processing reports...",
        QtConcurrent::run(dbManager, &DatabaseManager::getShiftReportStatsThread, date, shiftNo));
}

void MainWindow::getCashFloat(QDate date, int shiftNo)
{
    useProgressDialog("Processing reports...",
        QtConcurrent::run(dbManager, &DatabaseManager::getCashFloatThread, date, shiftNo));
}

void MainWindow::getMonthlyReport(int month, int year)
{
    useProgressDialog("Processing reports...",
        QtConcurrent::run(dbManager, &DatabaseManager::getMonthlyReportThread, month, year));
}

void MainWindow::updateDailyReportStats(QList<int> list, bool conn)
{
    if (!conn)
    {
        MainWindow::on_noDatabaseConnection();
    }
    else
    {
        ui->lbl_espCheckouts->setText(QString::number(list.at(0)));
        ui->lbl_totalCheckouts->setText(QString::number(list.at(1)));
        ui->lbl_espVacancies->setText(QString::number(list.at(2)));
        ui->lbl_totalVacancies->setText(QString::number(list.at(3)));
    }
}

void MainWindow::updateShiftReportStats(QStringList list, bool conn)
{
    if (!conn)
    {
        MainWindow::on_noDatabaseConnection();
    }
    else
    {
        ui->lbl_cashAmt->setText(list.at(0));
        ui->lbl_debitAmt->setText(list.at(1));
        ui->lbl_chequeAmt->setText(list.at(2));
        ui->lbl_depoAmt->setText(list.at(3));
        ui->lbl_shiftAmt->setText(list.at(4));    }
}

void MainWindow::updateCashFloat(QDate date, int shiftNo, QStringList list, bool conn)
{
    if (!conn)
    {
        MainWindow::on_noDatabaseConnection();
    }
    else
    {
        ui->lastModifiedEmpName_lbl->setText(list.at(0));
        ui->lastModifiedDateTime_lbl->setText(list.at(1));
        ui->pte_floatMemo->document()->setPlainText(list.at(2));
        ui->sbox_nickels->setValue(list.at(3).toInt());
        ui->sbox_dimes->setValue(list.at(4).toInt());
        ui->sbox_quarters->setValue(list.at(5).toInt());
        ui->sbox_loonies->setValue(list.at(6).toInt());
        ui->sbox_toonies->setValue(list.at(7).toInt());
        ui->sbox_fives->setValue(list.at(8).toInt());
        ui->sbox_tens->setValue(list.at(9).toInt());
        ui->sbox_twenties->setValue(list.at(10).toInt());
        ui->sbox_fifties->setValue(list.at(11).toInt());
        ui->sbox_hundreds->setValue(list.at(12).toInt());
        ui->lbl_floatAmt->setText(list.at(13));

        ui->cashFloatDate_lbl->setText(date.toString(Qt::ISODate));
        ui->cashFloatShift_lbl->setText(QString::number(shiftNo));
        ui->cashFloat_dateEdit->setDate(date);
        ui->cashFloat_spinBox->setValue(shiftNo);
    }
}

void MainWindow::on_saveFloat_btn_clicked()
{
    MainWindow::on_calculateTotal_btn_clicked();

    QDate date = ui->cashFloat_dateEdit->date();
    int shiftNo = ui->cashFloat_spinBox->value();
    QString empName = usernameLoggedIn;
    QString comments = ui->pte_floatMemo->toPlainText();
    QList<int> coins;
    coins << ui->sbox_nickels->value() << ui->sbox_dimes->value() << ui->sbox_quarters->value()
          << ui->sbox_loonies->value() << ui->sbox_toonies->value() << ui->sbox_fives->value()
          << ui->sbox_tens->value() << ui->sbox_twenties->value() << ui->sbox_fifties->value()
          << ui->sbox_hundreds->value();

    useProgressDialog("Saving Cash Float Changes ...", QtConcurrent::run(dbManager,
        &DatabaseManager::insertCashFloat, date, shiftNo, empName, comments, coins));
}

void MainWindow::on_calculateTotal_btn_clicked()
{
    double total = ui->sbox_nickels->value() * VAL_NICKEL;
    total += ui->sbox_dimes->value() * VAL_DIME;
    total += ui->sbox_quarters->value() * VAL_QUARTER;
    total += ui->sbox_loonies->value() * VAL_LOONIE;
    total += ui->sbox_toonies->value() * VAL_TOONIE;
    total += ui->sbox_fives->value() * VAL_FIVE;
    total += ui->sbox_tens->value() * VAL_TEN;
    total += ui->sbox_twenties->value() * VAL_TWENTY;
    total += ui->sbox_fifties->value() * VAL_FIFTY;
    total += ui->sbox_hundreds->value() * VAL_HUNDRED;

    ui->lbl_floatAmt->setText(QString("$ " + QString::number(total, 'f', 2)));
}

void MainWindow::on_cashFloatGo_btn_clicked()
{
    QDate date = ui->cashFloat_dateEdit->date();
    int shiftNo = ui->cashFloat_spinBox->value();

    MainWindow::getCashFloat(date, shiftNo);
}

void MainWindow::on_restrictionRefresh_btn_clicked()
{
    MainWindow::updateRestrictionTables();
}

void MainWindow::on_cashFloatCurrent_btn_clicked()
{
    ui->cashFloat_dateEdit->setDate(QDate::currentDate());
    ui->cashFloat_spinBox->setValue(currentshiftid);
    MainWindow::on_cashFloatGo_btn_clicked();
}

void MainWindow::updateCashFloatLastEditedLabels(QString empName,
    QString currentDateStr, QString currentTimeStr)
{
    ui->lastModifiedEmpName_lbl->setText(empName);
    ui->lastModifiedDateTime_lbl->setText(currentDateStr + " at " + currentTimeStr);
}

void MainWindow::on_monthlyReportGo_btn_clicked()
{
    int month = ui->month_comboBox->currentIndex() + 1;
    int year = ui->year_comboBox->currentText().toInt();

    MainWindow::getMonthlyReport(month, year);
}

void MainWindow::updateMonthlyReportUi(QStringList list, bool conn)
{
    if (!conn)
    {
        MainWindow::on_noDatabaseConnection();
    }
    else
    {
        ui->numBedsUsed_lbl->setText(list.at(0) + "/" + list.at(2));
        ui->numEmptyBeds_lbl->setText(list.at(1) + "/" + list.at(2));
        ui->numNewClients_lbl->setText(list.at(3));
        ui->numUniqueClients_lbl->setText(list.at(4));
        QString month = ui->month_comboBox->currentText();
        QString year = ui->year_comboBox->currentText();
        ui->monthlyReportMonth_lbl->setText(QString(month + " " + year));
    }
}

void MainWindow::on_noDatabaseConnection()
{
    statusBar()->showMessage(tr(""));
    QString msg = QString("\nCould not connect to the database.\n")
        + QString("\nPlease remember to save your changes when the connection to the database returns.\n")
        + QString("\nSelect \"File\" followed by the \"Connect to Database\" menu item to attempt to connect to the database.\n");
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("ArcWay");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setText(msg);
    msgBox.exec();
}


void MainWindow::on_noDatabaseConnection(QSqlDatabase* database)
{
    statusBar()->showMessage(tr(""));
    QString msg = QString("\nCould not connect to the database.\n")
        + QString("\nPlease remember to save your changes when the connection to the database returns.\n")
        + QString("\nSelect \"File\" followed by the \"Connect to Database\" menu item to attempt to connect to the database.\n");
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("ArcWay");
    QPushButton* reconnectButton = msgBox.addButton(tr("Re-connect"), QMessageBox::AcceptRole);
    msgBox.setStandardButtons(QMessageBox::Close);
    msgBox.setText(msg);
    msgBox.exec();

    if (msgBox.clickedButton() == reconnectButton)
    {
        statusBar()->showMessage(tr("Re-connecting"));
        dbManager->reconnectToDatabase(database);
    }
    else
    {
        statusBar()->showMessage(tr("No database connection"));
    }
}

void MainWindow::setReportsDateEditMax()
{
    ui->dailyReport_dateEdit->setMaximumDate(QDate::currentDate());
    ui->shiftReport_dateEdit->setMaximumDate(QDate::currentDate());
    ui->cashFloat_dateEdit->setMaximumDate(QDate::currentDate());
    ui->cashFloat_dateEdit->setMaximumDate(QDate::currentDate());
}

void MainWindow::on_reconnectedToDatabase()
{
    statusBar()->showMessage(tr("Database connection established"), 3000);
}
/*==============================================================================
REPORTS (END)
==============================================================================*/
void MainWindow::on_actionReconnect_to_Database_triggered()
{
    statusBar()->showMessage(tr("Re-connecting"));
    dbManager->reconnectToDatabase();
}

void MainWindow::on_actionBack_triggered()
{
    if (!backStack.isEmpty()){
        int page = backStack.pop();
        forwardStack.push(ui->stackedWidget->currentIndex());
        ui->stackedWidget->setCurrentIndex(page);
        ui->actionForward->setEnabled(true);

        switch(page) {
        case EDITBOOKING:
            on_btn_regCurDay_clicked();
            break;
        case CLIENTLOOKUP:
            ui->pushButton_bookRoom->setEnabled(false);
            ui->pushButton_processPaymeent->setEnabled(false);
            ui->pushButton_editClientInfo->setEnabled(false);
            ui->pushButton_CaseFiles->setEnabled(false);
        }
    }
}

void MainWindow::on_actionForward_triggered()
{
    if (!forwardStack.isEmpty()) {
        int page = forwardStack.pop();
        backStack.push(ui->stackedWidget->currentIndex());
        ui->stackedWidget->setCurrentIndex(page);
    }
}

void MainWindow::addHistory(int n){
    backStack.push(n);
    forwardStack.clear();
    ui->actionBack->setEnabled(true);
    ui->actionForward->setEnabled(false);
}

void MainWindow::on_pushButton_processPaymeent_clicked()
{
    addHistory(CLIENTLOOKUP);
    /*
    int nRow = ui->tableWidget_search_client->currentRow();
    if (nRow <0)
        return;

    curClient = new Client();
    curClientID = curClient->clientId = ui->tableWidget_search_client->item(nRow, 0)->text();
    curClient->fName =  ui->tableWidget_search_client->item(nRow, 1)->text();
    curClient->mName =  ui->tableWidget_search_client->item(nRow, 2)->text();
    curClient->lName =  ui->tableWidget_search_client->item(nRow, 3)->text();
    QString balanceString = ui->tableWidget_search_client->item(nRow, 5)->text();
    balanceString.replace("$", "");
    curClient->balance =  balanceString.toFloat();
    // curClient->balance =  ui->tableWidget_search_client->item(nRow, 5)->text().toFloat();
    curClient->fullName = QString(curClient->fName + " " + curClient->mName + " " + curClient->lName);
*/
    qDebug()<<"push processPayment";
    setSelectedClientInfo();
    trans = new transaction();
    tNew = true;
    QString note = "";
    payment * pay = new payment(this, trans, curClient->balance, 0 , curClient, note, true, usernameLoggedIn, QString::number(currentshiftid));
    pay->exec();
    delete(pay);

    //text receipt
    isRefund = trans->transType == "Refund" ? true : false;
    ui->actionReceipt->setEnabled(true);
    qDebug() << "total paid " << QString::number(trans->paidToday, 'f', 2);
    transType = trans->type;
    qDebug() << "trans type" << transType;
    if (transType != ""){
        saveReceipt(false, QString::number(trans->paidToday, 'f', 2), true);
        receiptid = "";
    }
//    createTextReceipt(QString::number(trans->amount),trans->type,QString::number(trans->paidToday),QString(),QString(),QString(),false,isRefund);
    on_pushButton_search_client_clicked();
}

void MainWindow::insertPcp(QTableWidget *tw, QString type){
    QString goal;
    QString strategy;
    QString date;

    int rows = tw->rowCount();
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < 3; j++) {
            if (tw->item(i,j)) {
                switch (j) {
                case 0: goal = tw->item(i,j)->text();
                    break;
                case 1: strategy = tw->item(i,j)->text();
                    break;
                case 2: date = tw->item(i,j)->text();
                }

              }
        }
//        QSqlQuery delResult = dbManager->deletePcpRow(i, type);
//        qDebug() << delResult.lastError();
        QSqlQuery addResult = dbManager->addPcp(i, curClientID, type, goal, strategy, date);
        qDebug() << "add pcp last error: "<< addResult.lastError();
    }
}

void MainWindow::on_btn_pcpRelaSave_clicked()
{
    insertPcp(ui->tw_pcpRela, "relationship");
}

void MainWindow::on_btn_pcpEduSave_clicked()
{
    insertPcp(ui->tw_pcpEdu, "educationEmployment");
}

void MainWindow::on_btn_pcpSubSave_clicked()
{
    insertPcp(ui->tw_pcpSub, "substanceUse");
}

void MainWindow::on_btn_pcpAccoSave_clicked()
{
    insertPcp(ui->tw_pcpAcco, "accomodationsPlanning");
}

void MainWindow::on_btn_pcpLifeSave_clicked()
{
    insertPcp(ui->tw_pcpLife, "lifeSkills");
}

void MainWindow::on_btn_pcpMentSave_clicked()
{
    insertPcp(ui->tw_pcpMent, "mentalHealth");
}

void MainWindow::on_btn_pcpPhySave_clicked()
{
    insertPcp(ui->tw_pcpPhy, "physicalHealth");
}

void MainWindow::on_btn_pcpLegSave_clicked()
{
    insertPcp(ui->tw_pcpLeg, "legalInvolvement");
}

void MainWindow::on_btn_pcpActSave_2_clicked()
{
    insertPcp(ui->tw_pcpAct, "activities");
}

void MainWindow::on_btn_pcpTradSave_clicked()
{
    insertPcp(ui->tw_pcpTrad, "traditions");
}

void MainWindow::on_btn_pcpOtherSave_clicked()
{
    insertPcp(ui->tw_pcpOther, "other");
}

void MainWindow::on_btn_pcpKeySave_clicked()
{
    insertPcp(ui->tw_pcpPpl, "people");
}

void MainWindow::on_actionPcptables_triggered()
{
    QString sin;
    if (ui->lineEdit_cl_SIN->text() != 0) {
        sin = ui->lineEdit_cl_SIN->text();
        QSqlQuery results = dbManager->checkUniqueSIN(sin);
        results.next();
        qDebug() << results.value(0).toString();
    }

}

void MainWindow::reloadPcpTable(int table){
    resetPcpTable(pcp_tables.at(table));
    useProgressDialog("Loading PCP tables...", QtConcurrent::run(this, retrievePcpData, pcpTypes.at(table), table));
}

void MainWindow::on_btn_pcpRelaUndo_clicked()
{
    qDebug() << "resetting table " << pcpTypes.at(0);
//    retrievePcpData(pcpTypes.at(0), 0);
    reloadPcpTable(0);
}

void MainWindow::on_btn_pcpEduUndo_clicked()
{
    qDebug() << "resetting table " << pcpTypes.at(1);
    reloadPcpTable(1);
}

void MainWindow::on_btn_pcpSubUndo_clicked()
{
    qDebug() << "resetting table " << pcpTypes.at(2);
    reloadPcpTable(2);
}

void MainWindow::on_btn_pcpAccoUndo_clicked()
{
    qDebug() << "resetting table " << pcpTypes.at(3);
    reloadPcpTable(3);
}

void MainWindow::on_btn_pcpLifeUndo_clicked()
{
    qDebug() << "resetting table " << pcpTypes.at(4);
    reloadPcpTable(4);
}

void MainWindow::on_btn_pcpMentUndo_clicked()
{
    qDebug() << "resetting table " << pcpTypes.at(5);
    reloadPcpTable(5);
}

void MainWindow::on_btn_pcpPhyUndo_2_clicked()
{
    qDebug() << "resetting table " << pcpTypes.at(6);
    reloadPcpTable(6);
}

void MainWindow::on_btn_pcpLegUndo_clicked()
{
    qDebug() << "resetting table " << pcpTypes.at(7);
    reloadPcpTable(7);
}

void MainWindow::on_btn_pcpActUndo_clicked()
{
    qDebug() << "resetting table " << pcpTypes.at(8);
    reloadPcpTable(8);
}

void MainWindow::on_btn_pcpTradUndo_clicked()
{
    qDebug() << "resetting table " << pcpTypes.at(9);
    reloadPcpTable(9);
}

void MainWindow::on_btn_pcpOtherUndo_clicked()
{
    qDebug() << "resetting table " << pcpTypes.at(10);
    reloadPcpTable(10);
}

void MainWindow::on_btn_pcpPplUndo_clicked()
{
    qDebug() << "resetting table " << pcpTypes.at(11);
    reloadPcpTable(11);
}

void MainWindow::on_btn_notesSave_clicked()
{
    QString notes = ui->te_notes->toPlainText();
    QSqlQuery result = dbManager->addNote(curClientID, notes);
    if (result.numRowsAffected() == 1) {
//        ui->lbl_noteWarning->setStyleSheet("QLabel#lbl_noteWarning {color = black;}");
//        ui->lbl_noteWarning->setText("Saved");
    } else {
//        ui->lbl_noteWarning->setStyleSheet("QLabel#lbl_noteWarning {color = red;}");
//        ui->lbl_noteWarning->setText(result.lastError().text());
        QSqlQuery result2 = dbManager->updateNote(curClientID, notes);
        qDebug() << "update note last error: " << result2.lastError();

    }
}

void MainWindow::on_btn_notesUndo_clicked()
{
    QSqlQuery result = dbManager->readNote(curClientID);
    qDebug() << "read note last error: "<< result.lastError();
    while (result.next()) {
        ui->te_notes->document()->setPlainText(result.value(0).toString());
    }
}

void MainWindow::on_btn_searchUsers_3_clicked()
{
    QString ename = ui->le_users_3->text();
    ui->tableWidget_5->setRowCount(0);
    ui->tableWidget_5->clear();
    //ui->tableWidget_5->horizontalHeader()->setStretchLastSection(true);

    QSqlQuery result = dbManager->execQuery("SELECT SpaceCode, cost, Monthly FROM Space WHERE SpaceCode LIKE '%"+ ename +"%' ORDER BY SpaceCode");

//    int numCols = result.record().count();
    ui->tableWidget_5->setColumnCount(8);
    ui->tableWidget_5->setHorizontalHeaderLabels(QStringList() << "Space Code" << "Building" << "Floor" << "Room" << "Space #" << "Type" << "Daily Cost" << "Monthly Cost");
    int x = 0;
    int qt = result.size();
    qDebug() << qt;
    while (result.next()) {
        // break down the spacecode

        QString spacecode = result.value(0).toString();
        std::string strspacecode = spacecode.toStdString();

        std::vector<std::string> brokenupspacecode = split(strspacecode, '-');
        // parse space code to check building number + floor number + room number + space number
        QString buildingnum = QString::fromStdString(brokenupspacecode[0]);
        QString floornum = QString::fromStdString(brokenupspacecode[1]);
        QString roomnum = QString::fromStdString(brokenupspacecode[2]);
        std::string bednumtype = brokenupspacecode[3];
        // strip the last character
        QString bednumber = QString::fromStdString(bednumtype.substr(0, bednumtype.size()-1));

        // get the last character to figure out the type
        char typechar = bednumtype[bednumtype.size()-1];
        QString thetype = "" + typechar;

    // get space id
//    QSqlQuery singlebedquery = dbManager->searchSingleBed(buildingnum, floornum, roomnum, bednumber);
//    singlebedquery.next();
//    QString spaceid = singlebedquery.value(3).toString();

        ui->tableWidget_5->insertRow(x);
        QStringList row;
        row << spacecode
            << buildingnum
            << floornum
            << roomnum
            << bednumber
            << QChar::fromLatin1(typechar)
            << result.value(1).toString()
            << result.value(2).toString();
        for (int i = 0; i < 8; ++i)
        {
            ui->tableWidget_5->setItem(x, i, new QTableWidgetItem(row.at(i)));
        }
        x++;
    }
    MainWindow::resizeTableView(ui->tableWidget_5);
    MainWindow::addCurrencyToTableWidget(ui->tableWidget_5, 6);
    MainWindow::addCurrencyToTableWidget(ui->tableWidget_5, 7);
}

void MainWindow::populate_modRoom_cboxes() {
    ui->cbox_roomLoc->clear();
    ui->cbox_roomFloor->clear();
    ui->cbox_roomRoom->clear();
    ui->cbox_roomType->clear();

    QSqlQuery buildings = dbManager->execQuery("SELECT BuildingNo FROM Building");
    ui->cbox_roomLoc->addItems(QStringList() << "");
    while (buildings.next()) {
        ui->cbox_roomLoc->addItems(QStringList() << buildings.value(0).toString());
    }
}

void MainWindow::on_btn_modRoomBdlg_clicked()
{
    curmodifyingspace = BUILDINGS;
    ui->editroommodifybox->clear();
    ui->editroommodifybox->setColumnCount(1);
    ui->editroommodifybox->setRowCount(0);
    ui->editroommodifybox->horizontalHeader()->setStretchLastSection(true);
    ui->editroommodifybox->setHorizontalHeaderLabels(QStringList() << "Building Number");

    // get all buildings
    QSqlQuery result = dbManager->execQuery("SELECT BuildingNo FROM Building");

    // populate table
    int x = 0;
    int qt = result.size();
    qDebug() << qt;
    while (result.next()) {
        QString buildingno = result.value(0).toString();
        ui->editroommodifybox->insertRow(x);
        QStringList row;
        row << buildingno;
        for (int i = 0; i < 1; ++i)
        {
            ui->editroommodifybox->setItem(x, i, new QTableWidgetItem(row.at(i)));
            ui->editroommodifybox->item(x, i)->setTextAlignment(Qt::AlignCenter);
        }
        x++;
    }
}

void MainWindow::on_btn_modRoomFloor_clicked()
{
    if (ui->cbox_roomLoc->currentText() == "") {
        ui->lbl_editUserWarning_3->setText("Please select a valid Building");
        return;
    }

    curmodifyingspace = FLOORS;
    ui->editroommodifybox->clear();
    ui->editroommodifybox->setColumnCount(1);
    ui->editroommodifybox->setRowCount(0);
    ui->editroommodifybox->horizontalHeader()->setStretchLastSection(true);
    ui->editroommodifybox->setHorizontalHeaderLabels(QStringList() << "Floor Number");
    // get building id
    QString building = ui->cbox_roomLoc->currentText();
    QSqlQuery qry = dbManager->execQuery("SELECT BuildingId FROM Building WHERE BuildingNo=" + building);

    qry.next();
    QString buildingid = qry.value(0).toString();

    QSqlQuery result = dbManager->execQuery("SELECT FloorNo FROM Floor WHERE BuildingId=" + buildingid);

    // populate table
    int x = 0;
    int qt = result.size();
    qDebug() << qt;
    while (result.next()) {
        QString buildingno = result.value(0).toString();
        ui->editroommodifybox->insertRow(x);
        QStringList row;
        row << buildingno;
        for (int i = 0; i < 1; ++i)
        {
            ui->editroommodifybox->setItem(x, i, new QTableWidgetItem(row.at(i)));
            ui->editroommodifybox->item(x, i)->setTextAlignment(Qt::AlignCenter);
        }
        x++;
    }
}

void MainWindow::on_btn_modRoomRoom_clicked()
{
    if (ui->cbox_roomLoc->currentText() == "") {
        ui->lbl_editUserWarning_3->setText("Please select a valid Building");
        return;
    }

    if (ui->cbox_roomFloor->currentText() == "") {
        ui->lbl_editUserWarning_3->setText("Please select a valid Floor");
        return;
    }

    curmodifyingspace = ROOMS;
    ui->editroommodifybox->clear();
    ui->editroommodifybox->setColumnCount(1);
    ui->editroommodifybox->setRowCount(0);
    ui->editroommodifybox->horizontalHeader()->setStretchLastSection(true);
    ui->editroommodifybox->setHorizontalHeaderLabels(QStringList() << "Room Number");

    // get building id
    QString building = ui->cbox_roomLoc->currentText();
    QSqlQuery qry = dbManager->execQuery("SELECT BuildingId FROM Building WHERE BuildingNo=" + building);

    qry.next();
    QString buildingid = qry.value(0).toString();

    // get Floor id
    QString floor = ui->cbox_roomFloor->currentText();
    QSqlQuery qry2 = dbManager->execQuery("SELECT FloorId FROM Floor WHERE BuildingId=" + buildingid + " AND FloorNo=" + floor);
    qDebug() << "SELECT FloorId FROM Floor WHERE BuildingId=" + building + " AND FloorNo=" + floor;
    qry2.next();

    QString floorid = qry2.value(0).toString();

    QSqlQuery result = dbManager->execQuery("SELECT RoomNo FROM Room WHERE FloorId=" + floorid);

    // populate table
    int x = 0;
    int qt = result.size();
    qDebug() << qt;
    while (result.next()) {
        QString buildingno = result.value(0).toString();
        ui->editroommodifybox->insertRow(x);
        QStringList row;
        row << buildingno;
        for (int i = 0; i < 1; ++i)
        {
            ui->editroommodifybox->setItem(x, i, new QTableWidgetItem(row.at(i)));
            ui->editroommodifybox->item(x, i)->setTextAlignment(Qt::AlignCenter);
        }
        x++;
    }
}

void MainWindow::on_btn_modRoomType_clicked()
{
    curmodifyingspace = TYPE;
    // this shouldn't be modifiable... - there are only ever beds and mats.
}

void MainWindow::on_EditShiftsButton_clicked()
{

   addHistory(ADMINPAGE);
   ui->stackedWidget->setCurrentIndex(EDITSHIFT);
   // ui->stackedWidget->setCurrentIndex(17);
   // showShifts(0);
   //return;
//    ui->tableWidget_6->clearContents();

    // populate table
    // ui->tableWidget_6

//    QSqlQuery shifts = dbManager->execQuery("SELECT * FROM Shift");
//    while (shifts.next()) {
//        int numberofshifts = shifts.value(11).toInt();
//        QString day = shifts.value(0).toString();
//        int dayrow = 0;
//        if (day == "Monday") {
//            dayrow = 0;
//        } else if (day == "Tuesday") {
//            dayrow = 1;
//        } else if (day == "Wednesday") {
//            dayrow = 2;
//        } else if (day == "Thursday") {
//            dayrow = 3;
//        } else if (day == "Friday") {
//            dayrow = 4;
//        } else if (day == "Saturday") {
//            dayrow = 5;
//        } else if (day == "Sunday") {
//            dayrow = 6;
//        }

//        for (int i = 1; i < (numberofshifts + 1); i++) {
//            QTime starttime = shifts.value((i*2)-1).toTime();
//            QTime endtime = shifts.value(i*2).toTime();

//            int starthr = starttime.hour();
//            int endhr = endtime.hour();

//            for (int j = starthr; j <= endhr; j++) {
//                QTableWidgetItem* item = new QTableWidgetItem();
//                QTableWidgetItem* temp = ui->tableWidget_6->item(dayrow, j);
//                QString newtxt = "";
//                if (temp != 0) {
//                    newtxt += temp->text();
//                }
//                newtxt += " " + QString::fromStdString(std::to_string(i)) + " ";
//                item->setText(newtxt);
//                ui->tableWidget_6->setItem(dayrow, j, item);
//            }
//        }
//    }

//    ui->comboBox_2->setCurrentIndex(1);
//    ui->comboBox_2->setCurrentIndex(0);
}

void MainWindow::on_cbox_roomLoc_currentTextChanged(const QString &arg1)
{
    qDebug() << arg1;
    // clear the other boxes
    // ui->cbox_roomLoc->clear();
    ui->cbox_roomFloor->clear();
    ui->cbox_roomRoom->clear();
    ui->cbox_roomType->clear();

    if (arg1 == "") {
        // empty selected, exit function
        return;
    }

    // get building id
    QString building = ui->cbox_roomLoc->currentText();
    QSqlQuery qry = dbManager->execQuery("SELECT BuildingId FROM Building WHERE BuildingNo=" + arg1);

    qry.next();

    // populate floor numbers based on selected building
    QSqlQuery floors = dbManager->execQuery("SELECT FloorNo FROM Floor WHERE BuildingId=" + qry.value(0).toString());

    ui->cbox_roomFloor->addItems(QStringList() << "");
    while (floors.next()) {
        ui->cbox_roomFloor->addItems(QStringList() << floors.value(0).toString());
        qDebug() << "added item" + floors.value(0).toString();
    }
}

void MainWindow::on_cbox_roomFloor_currentTextChanged(const QString &arg1)
{
    // ui->cbox_roomLoc->clear();
    // ui->cbox_roomFloor->clear();
    ui->cbox_roomRoom->clear();
    ui->cbox_roomType->clear();

    if (arg1 == "") {
        // empty selected, exit function
        return;
    }

    // get building id
    QString building = ui->cbox_roomLoc->currentText();
    QSqlQuery qry = dbManager->execQuery("SELECT BuildingId FROM Building WHERE BuildingNo=" + building);

    qry.next();

    qDebug() << "val:" << qry.value(0).toString();

    // populate room numbers based on selected floor
    // get floor id
    QSqlQuery qry2 = dbManager->execQuery("SELECT FloorId FROM Floor WHERE BuildingId=" + qry.value(0).toString() + " AND FloorNo=" + arg1);

    qry2.next();

    QString floorid = qry2.value(0).toString();
    qDebug() << "floorid:" + floorid;

    QSqlQuery rooms = dbManager->execQuery("SELECT RoomNo FROM Room WHERE FloorId=" + floorid);

    ui->cbox_roomRoom->addItems(QStringList() << "");
    while (rooms.next()) {
        ui->cbox_roomRoom->addItems(QStringList() << rooms.value(0).toString());
    }
}

void MainWindow::on_cbox_roomRoom_currentTextChanged(const QString &arg1)
{
    // ui->cbox_roomLoc->clear();
    // ui->cbox_roomFloor->clear();
    // ui->cbox_roomRoom->clear();
    ui->cbox_roomType->clear();

    if (arg1 == "") {
        // empty selected, exit function
        return;
    }

    QSqlQuery types = dbManager->execQuery("SELECT TypeDesc FROM Type");
    while (types.next()) {
        ui->cbox_roomType->addItems(QStringList() << types.value(0).toString());
    }
}

void MainWindow::on_cbox_roomType_currentTextChanged(const QString &arg1)
{

}

void MainWindow::on_cbox_roomType_currentIndexChanged(int index)
{

}


void MainWindow::on_tableWidget_search_client_itemClicked()
{
    ui->pushButton_bookRoom->setEnabled(true);
    ui->pushButton_processPaymeent->setEnabled(true);
    ui->pushButton_editClientInfo->setEnabled(true);
    ui->pushButton_CaseFiles->setEnabled(true);
}


void MainWindow::on_programDropdown_currentIndexChanged()
{
    clearTable(ui->bookingTable);
    ui->makeBookingButton->hide();
    ui->bookCostLabel->setText("0");
}

void MainWindow::on_confirmAddLunch_clicked()
{
    QString tmpStyleSheet = MainWindow::styleSheet();
    MainWindow::setStyleSheet("");
    
    MyCalendar* mc = new MyCalendar(this, curBook->startDate,curBook->endDate, curClient,1, curBook->room);
       mc->exec();
       delete(mc);

    MainWindow::setStyleSheet(tmpStyleSheet);
}

void MainWindow::on_confirmAddWake_clicked()
{
    QString tmpStyleSheet = MainWindow::styleSheet();
    MainWindow::setStyleSheet("");
    
    MyCalendar* mc = new MyCalendar(this, curBook->startDate,curBook->endDate, curClient,2, curBook->room);
        mc->exec();\
        delete(mc);

    MainWindow::setStyleSheet(tmpStyleSheet);
}

void MainWindow::on_editLunches_clicked()
{
    QString tmpStyleSheet = MainWindow::styleSheet();
    MainWindow::setStyleSheet("");

    ui->editUpdate->setEnabled(true);
    MyCalendar* mc;
    if(QDate::currentDate() < curBook->startDate){
        mc = new MyCalendar(this, curBook->startDate, curBook->endDate, curClient,1, curBook->room);
    }else{
       mc = new MyCalendar(this, QDate::currentDate(), curBook->endDate, curClient,1, curBook->room);
    }
       mc->setWindowTitle("Edit Lunches");
       mc->exec();
       delete(mc);

    MainWindow::setStyleSheet(tmpStyleSheet);
}

void MainWindow::on_editWakeup_clicked()
{
    QString tmpStyleSheet = MainWindow::styleSheet();
    MainWindow::setStyleSheet("");

    ui->editUpdate->setEnabled(true);
    MyCalendar* mc;
    if(QDate::currentDate() < curBook->startDate){

        mc = new MyCalendar(this, curBook->startDate,curBook->endDate, curClient,2, curBook->room);
    }else{

        mc = new MyCalendar(this, QDate::currentDate(),curBook->endDate, curClient,2,  curBook->room);
    }
        mc->setWindowTitle("Edit Wakeups");
        mc->exec();
        delete(mc);

    MainWindow::setStyleSheet(tmpStyleSheet);
}

void MainWindow::on_actionQuit_triggered()
{
    QApplication::quit();
}

void MainWindow::useProgressDialog(QString msg, QFuture<void> future){
    dialog->setLabelText(msg);
    futureWatcher.setFuture(future);
    dialog->setCancelButton(0);
    dialog->exec();
    futureWatcher.waitForFinished();
}

// room clicked
void MainWindow::on_tableWidget_5_clicked(const QModelIndex &index)
{
    ui->lbl_editUserWarning_3->setText("");
    // "ID Code" << "Building" << "Floor" << "Room" << "Bed Number" << "Type" << "Cost" << "Monthly"
    QString idcode = ui->tableWidget_5->model()->data(ui->tableWidget_5->model()->index(index.row(), 0)).toString();
    QString building = ui->tableWidget_5->model()->data(ui->tableWidget_5->model()->index(index.row(), 1)).toString();
    QString floor = ui->tableWidget_5->model()->data(ui->tableWidget_5->model()->index(index.row(), 2)).toString();
    QString room = ui->tableWidget_5->model()->data(ui->tableWidget_5->model()->index(index.row(), 3)).toString();
    QString bednumber = ui->tableWidget_5->model()->data(ui->tableWidget_5->model()->index(index.row(), 4)).toString();
    QString type = ui->tableWidget_5->model()->data(ui->tableWidget_5->model()->index(index.row(), 5)).toString();
    QString cost = ui->tableWidget_5->model()->data(ui->tableWidget_5->model()->index(index.row(), 6)).toString();
    QString monthly = ui->tableWidget_5->model()->data(ui->tableWidget_5->model()->index(index.row(), 7)).toString();

    // fill in stuff on the right
    populate_modRoom_cboxes();

    // set the building
    ui->cbox_roomLoc->setCurrentIndex(0);
    int currindex = 0;
    while (true) {
        if (ui->cbox_roomLoc->currentText() == building) {
            break;
        } else {
            currindex++;
            ui->cbox_roomLoc->setCurrentIndex(currindex);
        }
    }

    // set the floor
    int currindex2 = 0;
    while (true) {
        if (ui->cbox_roomFloor->currentText() == floor) {
            break;
        } else {
            currindex2++;
            ui->cbox_roomFloor->setCurrentIndex(currindex2);
        }
    }

    // set the Room
    int currindex3 = 0;
    while (true) {
        if (ui->cbox_roomRoom->currentText() == room) {
            break;
        } else {
            currindex3++;
            ui->cbox_roomRoom->setCurrentIndex(currindex3);
        }
    }

    // set the Type
    int currindex4 = 0;
    while (true) {
        if (ui->cbox_roomType->currentText().toStdString()[0] == type[0]) {
            break;
        } else {
            currindex4++;
            ui->cbox_roomType->setCurrentIndex(currindex4);
        }
    }

    // set the Space Number
    ui->le_roomNo->setText(bednumber);

    // set the prices
    // cost
    ui->doubleSpinBox->setValue(cost.toDouble());

    // monthly
    ui->doubleSpinBox_2->setValue(monthly.toDouble());
}

void MainWindow::on_lineEdit_search_clientName_returnPressed()
{
    MainWindow::on_pushButton_search_client_clicked();
}

void MainWindow::on_actionExport_to_PDF_triggered()
{
    QString rptTemplate;
    QtRPT *report = new QtRPT(this);

    // reports: daily
    if (ui->stackedWidget->currentIndex() == REPORTS && ui->swdg_reports->currentIndex() == DAILYREPORT){
        rptTemplate = ":/templates/pdf/daily.xml";
        report->recordCount << checkoutReport->model.rows
                            << vacancyReport->model.rows
                            << lunchReport->model.rows
                            << wakeupReport->model.rows;
    } else

    // reports: shift
    if (ui->stackedWidget->currentIndex() == REPORTS && ui->swdg_reports->currentIndex() == SHIFTREPORT){
        rptTemplate = ":/templates/pdf/shift.xml";
//        report->recordCount << bookingReport->model.rows
//                            << transactionReport->model.rows
//                            << clientLogReport->model.rows
//                            << otherReport->model.rows;
        report->recordCount << transactionReport->model.rows;

    } else

    // reports: float count
    if (ui->stackedWidget->currentIndex() == REPORTS && ui->swdg_reports->currentIndex() == FLOATCOUNT){
        rptTemplate = ":/templates/pdf/float.xml";
        report->recordCount << 1;
    } else

    // reports: monthly
    if (ui->stackedWidget->currentIndex() == REPORTS && ui->swdg_reports->currentIndex() == MONTHLYREPORT){
        rptTemplate = ":/templates/pdf/monthly.xml";
        report->recordCount << 1;
    } else

    // reports: restrictions
    if (ui->stackedWidget->currentIndex() == REPORTS && ui->swdg_reports->currentIndex() == RESTRICTIONS){
        rptTemplate = ":/templates/pdf/restriction.xml";
        report->recordCount << yellowReport->model.rows
                            << redReport->model.rows;
    } else

    // case files pcp
    if (ui->stackedWidget->currentIndex() == CASEFILE && ui->tabw_casefiles->currentIndex() == PERSIONACASEPLAN){
        rptTemplate = ":/templates/pdf/pcp.xml";
        for (auto x: pcp_tables) {
            report->recordCount << x->rowCount();
        }
    } else

    // case files running notes
    if (ui->stackedWidget->currentIndex() == CASEFILE && ui->tabw_casefiles->currentIndex() == RUNNINGNOTE){
        rptTemplate = ":/templates/pdf/running.xml";
        report->recordCount << (int) qCeil(ui->te_notes->document()->lineCount()/30.0);
        qDebug() << "record count: " << (int) qCeil(ui->te_notes->document()->lineCount()/30.0);
    } else

    // customer receipt
    if (ui->stackedWidget->currentIndex() == CONFIRMBOOKING ||
            ui->stackedWidget->currentIndex() == CLIENTLOOKUP ||
            ui->stackedWidget->currentIndex() == PAYMENTPAGE ||
            (ui->stackedWidget->currentIndex() == CASEFILE && ui->tabw_casefiles->currentIndex() == CL_RECEIPTS)) {
        rptTemplate = ":/templates/pdf/combinedRec.xml";
//        rptTemplate = ":/templates/pdf/staysummary.xml";
        report->recordCount << 1;
    } else

    // registry
    if (ui->stackedWidget->currentIndex() == EDITBOOKING) {
        rptTemplate = ":/templates/pdf/registry.xml";
        report->recordCount << ui->editLookupTable->rowCount();
        qDebug() << "rows: " << ui->editLookupTable->rowCount();
    }

    report->loadReport(rptTemplate);

    connect(report, SIGNAL(setValue(const int, const QString, QVariant&, const int)),
           this, SLOT(setValue(const int, const QString, QVariant&, const int)));

    report->printExec();
}

void MainWindow::setValue(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage) {

    // reports: daily
    if (ui->stackedWidget->currentIndex() == REPORTS && ui->swdg_reports->currentIndex() == DAILYREPORT){
        printDailyReport(recNo, paramName, paramValue, reportPage);
    } else

    // reports: shift
    if (ui->stackedWidget->currentIndex() == REPORTS && ui->swdg_reports->currentIndex() == SHIFTREPORT){
        printShiftReport(recNo, paramName, paramValue, reportPage);
    } else

    // reports: float count
    if (ui->stackedWidget->currentIndex() == REPORTS && ui->swdg_reports->currentIndex() == FLOATCOUNT){
        printFloatReport(recNo, paramName, paramValue, reportPage);
    } else

    // reports: monthly count
    if (ui->stackedWidget->currentIndex() == REPORTS && ui->swdg_reports->currentIndex() == MONTHLYREPORT){
        printMonthlyReport(recNo, paramName, paramValue, reportPage);
    } else

    // reports: restrictions
    if (ui->stackedWidget->currentIndex() == REPORTS && ui->swdg_reports->currentIndex() == RESTRICTIONS){
        printRestrictionReport(recNo, paramName, paramValue, reportPage);
    } else

    // case files pcp
    if (ui->stackedWidget->currentIndex() == CASEFILE && ui->tabw_casefiles->currentIndex() == PERSIONACASEPLAN){
        printPCP(recNo, paramName, paramValue, reportPage);
    }

    // case files running notes
    if (ui->stackedWidget->currentIndex() == CASEFILE && ui->tabw_casefiles->currentIndex() == RUNNINGNOTE){
        printRunningNotes(recNo, paramName, paramValue, reportPage);
    }

    // customer receipt
    if (ui->stackedWidget->currentIndex() == CONFIRMBOOKING ||
            ui->stackedWidget->currentIndex() == CLIENTLOOKUP ||
            ui->stackedWidget->currentIndex() == PAYMENTPAGE ||
             (ui->stackedWidget->currentIndex() == CASEFILE && ui->tabw_casefiles->currentIndex() == CL_RECEIPTS)) {
        printStaySummary(recNo, paramName, paramValue, reportPage);
    }

    // registry
    if (ui->stackedWidget->currentIndex() == EDITBOOKING) {
        printRegistry(recNo, paramName, paramValue, reportPage);
    }
}

void MainWindow::printDailyReport(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage){
    if (reportPage == 0){
        if (paramName == "client") {
            paramValue = checkoutReport->model.tableData->at(recNo * checkoutReport->model.cols + 0);
        } else if (paramName == "space") {
            paramValue = checkoutReport->model.tableData->at(recNo * checkoutReport->model.cols + 1);
        } else if (paramName == "start") {
            paramValue = checkoutReport->model.tableData->at(recNo * checkoutReport->model.cols + 2);
        } else if (paramName == "end") {
            paramValue = checkoutReport->model.tableData->at(recNo * checkoutReport->model.cols + 3);
        } else if (paramName == "program") {
            paramValue = checkoutReport->model.tableData->at(recNo * checkoutReport->model.cols + 4);
        } else if (paramName == "balance") {
            paramValue = checkoutReport->model.tableData->at(recNo * checkoutReport->model.cols + 5);
        } else if (paramName == "date") {
            paramValue = ui->dailyReport_dateEdit->text();
        } else if (paramName == "espout") {
            if (ui->lbl_espCheckouts->text().isEmpty()) return;
            paramValue = ui->lbl_espCheckouts->text();
        } else if (paramName == "espvac") {
            if (ui->lbl_espVacancies->text().isEmpty()) return;
            paramValue = ui->lbl_espVacancies->text();
        } else if (paramName == "totalout") {
            if (ui->lbl_totalCheckouts->text().isEmpty()) return;
            paramValue = ui->lbl_totalCheckouts->text();
        } else if (paramName == "totalvac") {
            if (ui->lbl_totalVacancies->text().isEmpty()) return;
            paramValue = ui->lbl_totalVacancies->text();
        } else if (paramName == "streetNo"){
             paramValue = getStreetNo();
             qDebug() << paramValue;
        } else if (paramName == "streetName"){
             paramValue = getStreetName();
             qDebug() << paramValue;
        } else if (paramName == "city"){
             paramValue = getCity();
             qDebug() << paramValue;
        } else if (paramName == "province"){
             paramValue = getProvince();
             qDebug() << paramValue;
        } else if (paramName == "zip"){
             paramValue = getZip();
             qDebug() << paramValue;
        } else if (paramName == "org"){
            paramValue = getOrgName();
            qDebug() << paramValue;
       }
    } else if (reportPage == 1) {
        if (paramName == "space"){
            paramValue = vacancyReport->model.tableData->at(recNo * vacancyReport->model.cols + 0);
        } else if (paramName == "prog"){
            paramValue = vacancyReport->model.tableData->at(recNo * vacancyReport->model.cols + 1);
        }
    } else if (reportPage == 2) {
        if (paramName == "client") {
            paramValue = lunchReport->model.tableData->at(recNo * lunchReport->model.cols + 0);
        } else if (paramName == "space") {
            paramValue = lunchReport->model.tableData->at(recNo * lunchReport->model.cols + 1);
        } else if (paramName == "lunches") {
            paramValue = lunchReport->model.tableData->at(recNo * lunchReport->model.cols + 2);
        }
    } else if (reportPage == 3) {
        if (paramName == "client") {
            paramValue = wakeupReport->model.tableData->at(recNo * wakeupReport->model.cols + 0);
        } else if (paramName == "space") {
            paramValue = wakeupReport->model.tableData->at(recNo * wakeupReport->model.cols + 1);
        } else if (paramName == "time") {
            paramValue = wakeupReport->model.tableData->at(recNo * wakeupReport->model.cols + 2);
        }
    }
}

void MainWindow::printShiftReport(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage){
    if (reportPage == 0){
        if (paramName == "client") {
            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 0);
        } else if (paramName == "trans") {
            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 1);
        } else if (paramName == "type") {
            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 2);
        } else if (paramName == "msdd") {
            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 4);
        } else if (paramName == "cno") {
            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 5);
        } else if (paramName == "cdate") {
            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 6);
        } else if (paramName == "status") {
            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 7);
        } else if (paramName == "deleted") {
            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 8);
        } else if (paramName == "employee") {
            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 9);
        } else if (paramName == "time") {
            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 10);
        } else if (paramName == "amt") {
            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 3);
//        if (paramName == "client") {
//            paramValue = bookingReport->model.tableData->at(recNo * bookingReport->model.cols + 0);
//        } else if (paramName == "space") {
//            paramValue = bookingReport->model.tableData->at(recNo * bookingReport->model.cols + 1);
//        } else if (paramName == "program") {
//            paramValue = bookingReport->model.tableData->at(recNo * bookingReport->model.cols + 2);
//        } else if (paramName == "start") {
//            paramValue = bookingReport->model.tableData->at(recNo * bookingReport->model.cols + 3);
//        } else if (paramName == "end") {
//            paramValue = bookingReport->model.tableData->at(recNo * bookingReport->model.cols + 4);
//        } else if (paramName == "action") {
//            paramValue = bookingReport->model.tableData->at(recNo * bookingReport->model.cols + 5);
//        } else if (paramName == "staff") {
//            paramValue = bookingReport->model.tableData->at(recNo * bookingReport->model.cols + 6);
//        } else if (paramName == "time") {
//            paramValue = bookingReport->model.tableData->at(recNo * bookingReport->model.cols + 7);
        } else if (paramName == "sname") {
            paramValue = userLoggedIn;
        } else if (paramName == "date") {
            paramValue = ui->lbl_shiftReportDateVal->text();
        } else if (paramName == "shiftNo") {
            paramValue = ui->lbl_shiftReportShiftVal->text();
        } else if (paramName == "cash") {
            paramValue = ui->lbl_cashAmt->text();
        } else if (paramName == "elec") {
            paramValue = ui->lbl_debitAmt->text();
        } else if (paramName == "cheque") {
            paramValue = ui->lbl_chequeAmt->text();
        } else if (paramName == "depo") {
            paramValue = ui->lbl_depoAmt->text();
        } else if (paramName == "total") {
            paramValue = ui->lbl_shiftAmt->text();
        } else if (paramName == "streetNo"){
             paramValue = getStreetNo();
             qDebug() << paramValue;
        } else if (paramName == "streetName"){
             paramValue = getStreetName();
             qDebug() << paramValue;
        } else if (paramName == "city"){
             paramValue = getCity();
             qDebug() << paramValue;
        } else if (paramName == "province"){
             paramValue = getProvince();
             qDebug() << paramValue;
        } else if (paramName == "zip"){
             paramValue = getZip();
             qDebug() << paramValue;
        } else if (paramName == "org"){
            paramValue = getOrgName();
            qDebug() << paramValue;
       }
//    } else if (reportPage == 1) {
//        if (paramName == "client") {
//            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 0);
//        } else if (paramName == "trans") {
//            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 1);
//        } else if (paramName == "type") {
//            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 2);
//        } else if (paramName == "msdd") {
//            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 4);
//        } else if (paramName == "cno") {
//            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 5);
//        } else if (paramName == "cdate") {
//            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 6);
//        } else if (paramName == "status") {
//            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 7);
//        } else if (paramName == "deleted") {
//            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 8);
//        } else if (paramName == "employee") {
//            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 9);
//        } else if (paramName == "time") {
//            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 10);
//        } else if (paramName == "amt") {
//            paramValue = transactionReport->model.tableData->at(recNo * transactionReport->model.cols + 3);
//        }
//    } else if (reportPage == 2) {
//        if (paramName == "client") {
//            paramValue = clientLogReport->model.tableData->at(recNo * clientLogReport->model.cols + 0);
//        } else if (paramName == "action") {
//            paramValue = clientLogReport->model.tableData->at(recNo * clientLogReport->model.cols + 1);
//        } else if (paramName == "employee") {
//            paramValue = clientLogReport->model.tableData->at(recNo * clientLogReport->model.cols + 2);
//        } else if (paramName == "time") {
//            paramValue = clientLogReport->model.tableData->at(recNo * clientLogReport->model.cols + 2);
//        }
//    } else if (reportPage == 3) {
//        if (paramName == "time") {
//            paramValue = otherReport->model.tableData->at(recNo * otherReport->model.cols + 0);
//        } else if (paramName == "employee") {
//            paramValue = otherReport->model.tableData->at(recNo * otherReport->model.cols + 1);
//        } else if (paramName == "log") {
//            paramValue = otherReport->model.tableData->at(recNo * otherReport->model.cols + 2);
//        }
    }
}

void MainWindow::printFloatReport(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage){
    Q_UNUSED(reportPage);
    Q_UNUSED(recNo);
    if (paramName == "nickel") {
        paramValue = ui->sbox_nickels->text();
    } else if (paramName == "dime") {
        paramValue = ui->sbox_dimes->text();
    } else if (paramName == "quarter") {
        paramValue = ui->sbox_quarters->text();
    } else if (paramName == "loonie") {
        paramValue = ui->sbox_loonies->text();
    } else if (paramName == "toonies") {
        paramValue = ui->sbox_toonies->text();
    } else if (paramName == "five") {
        paramValue = ui->sbox_fives->text();
    } else if (paramName == "ten") {
        paramValue = ui->sbox_tens->text();
    } else if (paramName == "twenty") {
        paramValue = ui->sbox_twenties->text();
    } else if (paramName == "fifty") {
        paramValue = ui->sbox_fifties->text();
    } else if (paramName == "hundred") {
        paramValue = ui->sbox_hundreds->text();
    } else if (paramName == "total") {
        paramValue = ui->lbl_floatAmt->text();
    } else if (paramName == "date") {
        paramValue = ui->cashFloatDate_lbl->text();
    } else if (paramName == "shift") {
        paramValue = ui->cashFloatShift_lbl->text();
    } else if (paramName == "comment") {
        paramValue = ui->pte_floatMemo->document()->toPlainText();
    } else if (paramName == "streetNo"){
         paramValue = getStreetNo();
         qDebug() << paramValue;
    } else if (paramName == "streetName"){
         paramValue = getStreetName();
         qDebug() << paramValue;
    } else if (paramName == "city"){
         paramValue = getCity();
         qDebug() << paramValue;
    } else if (paramName == "province"){
         paramValue = getProvince();
         qDebug() << paramValue;
    } else if (paramName == "zip"){
         paramValue = getZip();
         qDebug() << paramValue;
    } else if (paramName == "org"){
        paramValue = getOrgName();
        qDebug() << paramValue;
   }
}

void MainWindow::printMonthlyReport(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage){
    Q_UNUSED(reportPage);
    Q_UNUSED(recNo);
    if (paramName == "bedsUsed") {
        paramValue = ui->numBedsUsed_lbl->text();
    } else if (paramName == "bedsEmpty") {
        paramValue = ui->numEmptyBeds_lbl->text();
    } else if (paramName == "newClients") {
        paramValue = ui->numNewClients_lbl->text();
    } else if (paramName == "uniqueClients") {
        paramValue = ui->numUniqueClients_lbl->text();
    } else if (paramName == "date") {
        paramValue = ui->monthlyReportMonth_lbl->text();
    } else if (paramName == "streetNo"){
         paramValue = getStreetNo();
         qDebug() << paramValue;
    } else if (paramName == "streetName"){
         paramValue = getStreetName();
         qDebug() << paramValue;
    } else if (paramName == "city"){
         paramValue = getCity();
         qDebug() << paramValue;
    } else if (paramName == "province"){
         paramValue = getProvince();
         qDebug() << paramValue;
    } else if (paramName == "zip"){
         paramValue = getZip();
         qDebug() << paramValue;
    } else if (paramName == "org"){
        paramValue = getOrgName();
        qDebug() << paramValue;
   }
}

void MainWindow::printRestrictionReport(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage){
    Q_UNUSED(paramName);
    if (reportPage == 0) {
        paramValue = yellowReport->model.tableData->at(recNo * yellowReport->model.cols);
        if (paramName == "streetNo"){
             paramValue = getStreetNo();
             qDebug() << paramValue;
        } else if (paramName == "streetName"){
             paramValue = getStreetName();
             qDebug() << paramValue;
        } else if (paramName == "city"){
             paramValue = getCity();
             qDebug() << paramValue;
        } else if (paramName == "province"){
             paramValue = getProvince();
             qDebug() << paramValue;
        } else if (paramName == "zip"){
             paramValue = getZip();
             qDebug() << paramValue;
        } else if (paramName == "org"){
            paramValue = getOrgName();
            qDebug() << paramValue;
       }
    } else if (reportPage == 1) {
        paramValue = redReport->model.tableData->at(recNo * redReport->model.cols);
    }
}

void MainWindow::printPCP(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage) {
    qDebug() << "report page: " << reportPage;

    QTableWidget *tw_pcp = pcp_tables.at(reportPage);

    if (paramName == "goal") {
        if (tw_pcp->item(recNo, 0) == 0 ) return;
         paramValue = tw_pcp->item(recNo, 0)->text();
         qDebug() << "report page: " << reportPage << " recNp: " << recNo << " value: " << tw_pcp->item(recNo, 0)->text();
    } else if (paramName == "strategy") {
        if (tw_pcp->item(recNo, 1) == 0 ) return;
         paramValue = tw_pcp->item(recNo, 1)->text();
         qDebug() << "report page: " << reportPage << " recNp: " << recNo << " value: " << tw_pcp->item(recNo, 1)->text();
    } else if (paramName == "date") {
        if (tw_pcp->item(recNo, 2) == 0 ) return;
         paramValue = tw_pcp->item(recNo, 2)->text();
         qDebug() << "report page: " << reportPage << " recNp: " << recNo << " value: " << tw_pcp->item(recNo, 2)->text();
    } else if (paramName == "person") {
        if (tw_pcp->item(recNo, 0) == 0 ) return;
         paramValue = tw_pcp->item(recNo, 0)->text();
         qDebug() << "report page: " << reportPage << " recNp: " << recNo << " value: " << tw_pcp->item(recNo, 0)->text();
    } else if (paramName == "task"){
        if (tw_pcp->item(recNo, 1) == 0 ) return;
         paramValue = tw_pcp->item(recNo, 1)->text();
         qDebug() << "report page: " << reportPage << " recNp: " << recNo << " value: " << tw_pcp->item(recNo, 1)->text();
    }
}

void MainWindow::printRunningNotes(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage) {
    Q_UNUSED(reportPage);
    Q_UNUSED(paramName);
    QString s;
    QTextCursor *cursor = new QTextCursor(ui->te_notes->document());

    qDebug() << "recNo: " << recNo;

    cursor->movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, recNo * 30);

    for (int i = 0; i < 30; i++) {
        cursor->movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        s += cursor->selectedText() + "\r\n";
        if (!cursor->movePosition(QTextCursor::Down, QTextCursor::MoveAnchor)) break;
        cursor->movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    }
    s.chop(2);
    paramValue = s;
}

void MainWindow::printStaySummary(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage) {
    Q_UNUSED(recNo);
    Q_UNUSED(reportPage);

    if (paramName == "receiptid") {
        paramValue = curReceipt[0];
    }

    if (paramName == "date") {
        paramValue = curReceipt[1];
    }

    if (paramName == "time") {
        paramValue = curReceipt[2];
    }

    if (paramName == "lastFirst") {
        paramValue = curReceipt[3];

    } else if (paramName == "start") {
        paramValue = curReceipt[4];

    } else if (paramName == "end") {
        paramValue = curReceipt[5];

    } else if (paramName == "numNights") {
        paramValue = curReceipt[6];

    } else if (paramName == "bedType") {
        paramValue = curReceipt[7];

    } else if (paramName == "roomNo") {
        paramValue = curReceipt[8];

    } else if (paramName == "prog") {
        paramValue = curReceipt[9];

    } else if (paramName == "desc") {
        paramValue = curReceipt[10];

    } else if (paramName == "streetNo"){
        paramValue = curReceipt[11];

    } else if (paramName == "streetName"){
        paramValue = curReceipt[12];

    } else if (paramName == "city"){
        paramValue = curReceipt[13];

    } else if (paramName == "province"){
        paramValue = curReceipt[14];

    } else if (paramName == "zip"){
        paramValue = curReceipt[15];

    } else if (paramName == "org"){
        paramValue = curReceipt[16];

    } else if (paramName == "totalCost") {
        paramValue = curReceipt[17];

    } else if (paramName == "payType") {
        paramValue = curReceipt[18];

    } else if (paramName == "totalPaid") {
        paramValue = curReceipt[19] == "$0.00" ? "$0.00" : curReceipt[20] + " of " + curReceipt[19];

    } else if (paramName == "owing") {
        paramValue = curReceipt[21];
    }
}

void MainWindow::printRegistry(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage) {
    Q_UNUSED(reportPage);
    qDebug() << "params: recNo: " << recNo << " paramName: " << paramName << " paramValue: " << paramValue << " reportPage: " << reportPage;

    if (paramName == "client") {
        qDebug() << "param client";
        if (ui->editLookupTable->item(recNo, 0) == 0 ) return;
         paramValue = ui->editLookupTable->item(recNo, 0)->text();
         qDebug() << "printed col 0";

    } else if (paramName == "space") {
        qDebug() << "param space";
        if (ui->editLookupTable->item(recNo, 1) == 0 ) return;
         paramValue = ui->editLookupTable->item(recNo, 1)->text();
         qDebug() << "printed col 1";

    } else if (paramName == "start") {
        qDebug() << "param start";
        if (ui->editLookupTable->item(recNo, 2) == 0 ) return;
         paramValue = ui->editLookupTable->item(recNo, 2)->text();
         qDebug() << "printed col 2";

    } else if (paramName == "end") {
        qDebug() << "param end";
        if (ui->editLookupTable->item(recNo, 3) == 0 ) return;
         paramValue = ui->editLookupTable->item(recNo, 3)->text();
         qDebug() << "printed col 3";

    } else if (paramName == "prog"){
        qDebug() << "param prog";
        if (ui->editLookupTable->item(recNo, 4) == 0 ) return;
         paramValue = ui->editLookupTable->item(recNo, 4)->text();
         qDebug() << "printed col 4";

//    } else if (paramName == "cost"){
//        qDebug() << "param cost";
//        if (ui->editLookupTable->item(recNo, 5) == 0 ) return;
//         paramValue = ui->editLookupTable->item(recNo, 5)->text();
//         qDebug() << "printed col 5";

//    } else if (paramName == "rate"){
//        qDebug() << "param rate";
//        if (ui->editLookupTable->item(recNo, 6) == 0 ) return;
//         paramValue = ui->editLookupTable->item(recNo, 6)->text();
//         qDebug() << "printed col 6";

    } else if (paramName == "streetNo"){
         paramValue = getStreetNo();
         qDebug() << paramValue;
    } else if (paramName == "streetName"){
         paramValue = getStreetName();
         qDebug() << paramValue;
    } else if (paramName == "city"){
         paramValue = getCity();
         qDebug() << paramValue;
    } else if (paramName == "province"){
         paramValue = getProvince();
         qDebug() << paramValue;
    } else if (paramName == "zip"){
         paramValue = getZip();
         qDebug() << paramValue;
    } else if (paramName == "org"){
        paramValue = getOrgName();
        qDebug() << paramValue;
   }
}

void MainWindow::on_btn_createNewUser_3_clicked()
{
    ui->lbl_editUserWarning_3->setText("");
    // add new vacancy
    QString building = ui->cbox_roomLoc->currentText();
    QString floor = ui->cbox_roomFloor->currentText();
    QString room = ui->cbox_roomRoom->currentText();
    QString bednumber = ui->le_roomNo->text();
    QString type = ui->cbox_roomType->currentText();
    QString cost = QString::number(ui->doubleSpinBox->value());
    QString monthly = QString::number(ui->doubleSpinBox_2->value());

    // check if it already exists
    QSqlQuery result = dbManager->searchSingleBed(building, floor, room, bednumber);

    if (!result.next()) {
        // doesn't exist so make one
        // INSERT INTO Space
        // VALUES (7, 25, '', 'Mat', 0, 0, NULL);

        // get building id
        QSqlQuery bid = dbManager->execQuery("SELECT BuildingId FROM Building WHERE BuildingNo=" + building);
        bid.next();

        // get floor id
        QSqlQuery fid = dbManager->execQuery("SELECT FloorId FROM Floor WHERE FloorNo=" + floor + " AND BuildingId=" + bid.value(0).toString());
        fid.next();

        // get room id
        QSqlQuery rid = dbManager->execQuery("SELECT RoomId FROM Room WHERE FloorId=" + fid.value(0).toString() + " AND RoomNo=" + room);
        rid.next();

        QString roomid = rid.value(0).toString();
        qDebug() << roomid;
        QSqlQuery insertres = dbManager->execQuery("INSERT INTO Space "
                                                   "VALUES (" + bednumber +
                                                   ", " + roomid +
                                                   ", '', " + "'" + type +
                                                   "', " + cost +
                                                   ", " + monthly +
                                                   ", NULL)");
        qDebug() <<"LMFAO: INSERT INTO Space "
                                     "VALUES (" + bednumber +
                                     ", " + roomid +
                                     ", '', " + "'" + type +
                                     "', " + cost +
                                     ", " + monthly +
                                     ", NULL)";
        // update spacecodes
        dbManager->updateAllSpacecodes();
        // clear everything
//        ui->cbox_roomLoc->clear();
//        ui->cbox_roomFloor->clear();
//        ui->cbox_roomRoom->clear();
        ui->le_roomNo->clear();
        // ui->cbox_roomType->clear();
        ui->doubleSpinBox->setValue(0.0);
        ui->doubleSpinBox_2->setValue(0.0);
        ui->tableWidget_5->clear();
        ui->tableWidget_5->setRowCount(0);
        ui->le_users_3->clear();
        // set horizontal headers
        ui->tableWidget_5->setColumnCount(8);
        ui->tableWidget_5->setHorizontalHeaderLabels(QStringList() << "ID Code" << "Building" << "Floor" << "Room" << "Bed Number" << "Type" << "Cost" << "Monthly");
        // populate_modRoom_cboxes();
        ui->lbl_editUserWarning_3->setText("Vacancy created");
        // list all
        on_btn_listAllUsers_3_clicked();
    } else {
        ui->lbl_editUserWarning_3->setText("This vacancy already exists. Please change the bed number.");
    }
}

void MainWindow::on_pushButton_14_clicked()
{
    ui->lbl_editUserWarning_3->setText("");

    // update vacancy
    QString building = ui->cbox_roomLoc->currentText();
    QString floor = ui->cbox_roomFloor->currentText();
    QString room = ui->cbox_roomRoom->currentText();
    QString bednumber = ui->le_roomNo->text();
    QString type = ui->cbox_roomType->currentText();
    QString cost = QString::number(ui->doubleSpinBox->value());
    QString monthly = QString::number(ui->doubleSpinBox_2->value());

    // check to make sure it exists
    QSqlQuery result = dbManager->searchSingleBed(building, floor, room, bednumber);

    if (result.next()) {
        // doesn't exist so make one
        // INSERT INTO Space
        // VALUES (7, 25, '', 'Mat', 0, 0, NULL);

        // get room id
        QSqlQuery idinfo = dbManager->searchIDInformation(building, floor, room);

        idinfo.next();

        QString roomid = idinfo.value(0).toString();

        QSqlQuery updateres = dbManager->execQuery("UPDATE s "
                                                   "SET s.cost = " + cost +
                                                   ", s.Monthly = " + monthly + " "
                                                   "FROM Space s INNER JOIN Room r ON s.RoomId = r.RoomId "
                                                   "INNER JOIN Floor f ON r.FloorId = f.FloorId "
                                                   "INNER JOIN Building b ON f.BuildingId = b.BuildingId "
                                                   "WHERE b.BuildingNo = " + building + " "
                                                   "AND f.FloorNo = " + floor + " "
                                                   "AND r.RoomNo = " + room + " "
                                                   "AND s.SpaceNo = " + bednumber);
        // update spacecodes
        dbManager->updateAllSpacecodes();

        populate_modRoom_cboxes();

        ui->lbl_editUserWarning_3->setText("Vacancy updated");
        // clear everything
        ui->cbox_roomLoc->clear();
        ui->cbox_roomFloor->clear();
        ui->cbox_roomRoom->clear();
        ui->le_roomNo->clear();
        ui->cbox_roomType->clear();
        ui->le_roomNo->clear();
        ui->doubleSpinBox->setValue(0.0);
        ui->doubleSpinBox_2->setValue(0.0);
        ui->tableWidget_5->clear();
        ui->tableWidget_5->setRowCount(0);
        ui->le_users_3->clear();
        // set horizontal headers
        ui->tableWidget_5->setColumnCount(8);
        ui->tableWidget_5->setHorizontalHeaderLabels(QStringList() << "ID Code" << "Building" << "Floor" << "Room" << "Bed Number" << "Type" << "Cost" << "Monthly");
        populate_modRoom_cboxes();
    } else {
        ui->lbl_editUserWarning_3->setText("This vacancy doesn't exist. Please select a valid vacancy.");
    }
}

void MainWindow::on_editroommodifybox_clicked(const QModelIndex &index)
{
    // populate edittext on the bottom
    QString txt = ui->editroommodifybox->item(index.row(), 0)->text();
    ui->le_newTypeLoc->setText(txt);
}

void MainWindow::on_btn_newTypeLoc_clicked()
{
    QSqlQuery idinfo;

    // modify add new
    switch (curmodifyingspace) {
    case BUILDINGS: {
        QSqlQuery q1 = dbManager->execQuery("SELECT * FROM Building WHERE BuildingNo=" + ui->le_newTypeLoc->text());
        if (q1.next()) {
            ui->lbl_editUserWarning_3->setText("A building with this number already exists.");
        } else {
            QSqlQuery q2 = dbManager->execQuery("INSERT INTO Building VALUES(" + ui->le_newTypeLoc->text() + ", '')");
            ui->lbl_editUserWarning_3->setText("Building Created");

            ui->cbox_roomLoc->clear();
            ui->cbox_roomFloor->clear();
            ui->cbox_roomRoom->clear();
            ui->le_roomNo->clear();
            ui->cbox_roomType->clear();
            populate_modRoom_cboxes();
            on_btn_modRoomBdlg_clicked();
        }
        break;
    }
    case FLOORS: {
//        idinfo = dbManager->execQuery("SELECT r.RoomId, b.BuildingId, f.FloorId"
//                                      "FROM Space s INNER JOIN Room r ON s.RoomId = r.RoomId INNER JOIN Floor f ON r.FloorId = f.FloorId "
//                                      "INNER JOIN Building b ON f.BuildingId = b.BuildingId "
//                                      "WHERE b.BuildingNo =" + ui->cbox_roomLoc->currentText() + " ");
//                                      "AND f.FloorNo =" + floorno + " "
//                                      "AND r.RoomNo =" + roomno + " ");
        //idinfo = dbManager->searchIDInformation(ui->cbox_roomLoc->currentText(), "1", "0");

        QSqlQuery buildingidq = dbManager->execQuery("SELECT BuildingId FROM Building WHERE BuildingNo=" + ui->cbox_roomLoc->currentText());
        buildingidq.next();
        QSqlQuery q3 = dbManager->execQuery("SELECT * FROM Floor WHERE FloorNo=" + ui->le_newTypeLoc->text() +
                                            " AND BuildingId = " + buildingidq.value(0).toString()); // room building floor

        if (q3.next()) {
            ui->lbl_editUserWarning_3->setText("A floor with this number already exists.");
        } else {
            QSqlQuery q4 = dbManager->execQuery("INSERT INTO Floor VALUES (" + ui->le_newTypeLoc->text()
                                                + ", " + buildingidq.value(0).toString() + ")");
            ui->lbl_editUserWarning_3->setText("Floor Created");

            ui->cbox_roomFloor->clear();
            ui->cbox_roomRoom->clear();
            ui->le_roomNo->clear();
            ui->cbox_roomType->clear();

            // fake an update to the building cbox to refresh the dropdown
            int before = ui->cbox_roomLoc->currentIndex();
            ui->cbox_roomLoc->setCurrentIndex(0);
            ui->cbox_roomLoc->setCurrentIndex(before);
            on_btn_modRoomFloor_clicked();
        }

        break;
    }
    case ROOMS: {
        QSqlQuery buildingidq = dbManager->execQuery("SELECT BuildingId FROM Building WHERE BuildingNo=" + ui->cbox_roomLoc->currentText());
        buildingidq.next();
//        idinfo = dbManager->searchIDInformation(ui->cbox_roomLoc->currentText(), ui->cbox_roomFloor->currentText(), "0");
//        idinfo.next();
        QSqlQuery q3 = dbManager->execQuery("SELECT * FROM Floor WHERE FloorNo=" + ui->cbox_roomFloor->currentText() +
                                            " AND BuildingId = " + buildingidq.value(0).toString()); // room building floor
        q3.next();

        qDebug() << "Bid" + buildingidq.value(0).toString();

        QString floorid = q3.value(0).toString();

        qDebug() << "fid" + floorid;

        QSqlQuery q4 = dbManager->execQuery("SELECT * FROM Room WHERE RoomNo=" + ui->le_newTypeLoc->text()
                                            + " AND FloorId =" + floorid);

        if (q4.next()) {
            ui->lbl_editUserWarning_3->setText("A room with this number already exists.");
        } else {
            dbManager->execQuery("INSERT INTO Room VALUES (" + floorid + ", " + ui->le_newTypeLoc->text() + ")");
            ui->lbl_editUserWarning_3->setText("Room created");

            ui->cbox_roomRoom->clear();
            ui->le_roomNo->clear();
            ui->cbox_roomType->clear();

            // fake an update to the floors cbox to refresh the dropdown
            int before = ui->cbox_roomFloor->currentIndex();
            ui->cbox_roomFloor->setCurrentIndex(0);
            ui->cbox_roomFloor->setCurrentIndex(before);
            on_btn_modRoomRoom_clicked();
        }

        break;
    }
    case NOT_SET: {
        ui->lbl_editUserWarning_3->setText("You haven't selected anything to modify yet");
        break;
    }
    default: {
        break;
    }
    }
    // clear everything
    ui->doubleSpinBox->setValue(0.0);
    ui->doubleSpinBox_2->setValue(0.0);
    ui->tableWidget_5->clear();
    ui->tableWidget_5->setRowCount(0);
    ui->le_users_3->clear();
    // set horizontal headers
    ui->tableWidget_5->setColumnCount(8);
    ui->tableWidget_5->setHorizontalHeaderLabels(QStringList() << "ID Code" << "Building" << "Floor" << "Room" << "Bed Number" << "Type" << "Cost" << "Monthly");
    ui->le_newTypeLoc->clear();
}

void MainWindow::on_btn_delTypeLoc_clicked()
{
    QSqlQuery idinfo;

    // modify delete
    switch (curmodifyingspace) {
    case BUILDINGS: {
        QSqlQuery q1 = dbManager->execQuery("SELECT * FROM Building WHERE BuildingNo=" + ui->le_newTypeLoc->text());
        if (!q1.next()) {
            ui->lbl_editUserWarning_3->setText("No building with this number exists.");
        } else {
            // check if floors exist in this building
            QSqlQuery buildingidq = dbManager->execQuery("SELECT BuildingId FROM Building WHERE BuildingNo=" + ui->cbox_roomLoc->currentText());
            buildingidq.next();
            QSqlQuery check = dbManager->execQuery("SELECT * FROM Floor WHERE BuildingId=" + buildingidq.value(0).toString());

            if (check.next()) {
                ui->lbl_editUserWarning_3->setText("Delete failed - There are still floors in this building");
            } else {
                QSqlQuery q2 = dbManager->execQuery("DELETE FROM Building WHERE BuildingNo=" + ui->le_newTypeLoc->text());
                ui->lbl_editUserWarning_3->setText("Building Deleted");

                ui->cbox_roomLoc->clear();
                ui->cbox_roomFloor->clear();
                ui->cbox_roomRoom->clear();
                ui->le_roomNo->clear();
                ui->cbox_roomType->clear();
                populate_modRoom_cboxes();
                on_btn_modRoomBdlg_clicked();
            }
        }
        break;
    }
    case FLOORS: {
        QSqlQuery buildingidq = dbManager->execQuery("SELECT BuildingId FROM Building WHERE BuildingNo=" + ui->cbox_roomLoc->currentText());
        buildingidq.next();
        QSqlQuery q3 = dbManager->execQuery("SELECT * FROM Floor WHERE FloorNo=" + ui->le_newTypeLoc->text() +
                                            " AND BuildingId = " + buildingidq.value(0).toString()); // room building floor

        if (!q3.next()) {
            ui->lbl_editUserWarning_3->setText("No floor with this number exists.");
        } else {
            // check if rooms exist
            QSqlQuery check = dbManager->execQuery("SELECT * FROM Room WHERE FloorId=" + q3.value(0).toString());
            if (check.next()) {
                ui->lbl_editUserWarning_3->setText("Delete failed - There are still rooms on this floor");
            } else {
                QSqlQuery q4 = dbManager->execQuery("DELETE FROM Floor WHERE FloorNo=" + ui->le_newTypeLoc->text()
                                                    + " AND BuildingId=" + buildingidq.value(0).toString());
                ui->lbl_editUserWarning_3->setText("Floor Deleted");

                ui->cbox_roomFloor->clear();
                ui->cbox_roomRoom->clear();
                ui->le_roomNo->clear();
                ui->cbox_roomType->clear();

                // fake an update to the building cbox to refresh the dropdown
                int before = ui->cbox_roomLoc->currentIndex();
                ui->cbox_roomLoc->setCurrentIndex(0);
                ui->cbox_roomLoc->setCurrentIndex(before);
                on_btn_modRoomFloor_clicked();
            }
        }

        break;
    }
    case ROOMS: {
        QSqlQuery buildingidq = dbManager->execQuery("SELECT BuildingId FROM Building WHERE BuildingNo=" + ui->cbox_roomLoc->currentText());
        buildingidq.next();
//        idinfo = dbManager->searchIDInformation(ui->cbox_roomLoc->currentText(), ui->cbox_roomFloor->currentText(), "0");
//        idinfo.next();
        QSqlQuery q3 = dbManager->execQuery("SELECT * FROM Floor WHERE FloorNo=" + ui->cbox_roomFloor->currentText() +
                                            " AND BuildingId = " + buildingidq.value(0).toString()); // room building floor
        q3.next();

        QString floorid = q3.value(0).toString();

        qDebug() << floorid;

        QSqlQuery q4 = dbManager->execQuery("SELECT * FROM Room WHERE RoomNo=" + ui->le_newTypeLoc->text()
                                            + " AND FloorId =" + floorid);

        if (!q4.next()) {
            ui->lbl_editUserWarning_3->setText("No room with this number exists.");
        } else {
            QSqlQuery check = dbManager->execQuery("SELECT * FROM Space WHERE RoomId=" + q4.value(0).toString());

            if (check.next()) {
                ui->lbl_editUserWarning_3->setText("Delete failed - There are still beds in this room");
            } else {
                dbManager->execQuery("DELETE FROM Room WHERE FloorId=" + floorid + " AND RoomNo=" + ui->le_newTypeLoc->text());
                ui->lbl_editUserWarning_3->setText("Room Deleted");

                ui->cbox_roomRoom->clear();
                ui->le_roomNo->clear();
                ui->cbox_roomType->clear();

                // fake an update to the floors cbox to refresh the dropdown
                int before = ui->cbox_roomFloor->currentIndex();
                ui->cbox_roomFloor->setCurrentIndex(0);
                ui->cbox_roomFloor->setCurrentIndex(before);
                on_btn_modRoomRoom_clicked();
            }
        }
        break;
    }
    case NOT_SET: {
        break;
    }
    default: {
        break;
    }
    }
    // clear everything
    ui->le_roomNo->clear();
    ui->doubleSpinBox->setValue(0.0);
    ui->doubleSpinBox_2->setValue(0.0);
    ui->tableWidget_5->clear();
    ui->tableWidget_5->setRowCount(0);
    ui->le_users_3->clear();
    // set horizontal headers
    ui->tableWidget_5->setColumnCount(8);
    ui->tableWidget_5->setHorizontalHeaderLabels(QStringList() << "ID Code" << "Building" << "Floor" << "Room" << "Bed Number" << "Type" << "Cost" << "Monthly");
    ui->le_newTypeLoc->clear();
}

void MainWindow::on_pushButton_15_clicked()
{
    ui->lbl_editUserWarning_3->setText("");
    // delete space
    QString building = ui->cbox_roomLoc->currentText();
    QString floor = ui->cbox_roomFloor->currentText();
    QString room = ui->cbox_roomRoom->currentText();
    QString bednumber = ui->le_roomNo->text();
    QString type = ui->cbox_roomType->currentText();
    QString cost = QString::number(ui->doubleSpinBox->value());
    QString monthly = QString::number(ui->doubleSpinBox_2->value());

    // check to make sure it exists
    // check if it already exists
    QSqlQuery result = dbManager->searchSingleBed(building, floor, room, bednumber);

    if (result.next()) {
        // doesn't exist so make one
        QSqlQuery deleteres = dbManager->deleteSpace(building, floor, room, bednumber);

        populate_modRoom_cboxes();

        ui->lbl_editUserWarning_3->setText("Vacancy Deleted");

        // clear everything
        ui->cbox_roomLoc->clear();
        ui->cbox_roomFloor->clear();
        ui->cbox_roomRoom->clear();
        ui->le_roomNo->clear();
        ui->cbox_roomType->clear();
        ui->le_roomNo->clear();
        ui->doubleSpinBox->setValue(0.0);
        ui->doubleSpinBox_2->setValue(0.0);
        ui->tableWidget_5->clear();
        ui->tableWidget_5->setRowCount(0);
        ui->le_users_3->clear();
        // set horizontal headers
        ui->tableWidget_5->setColumnCount(8);
        ui->tableWidget_5->setHorizontalHeaderLabels(QStringList() << "ID Code" << "Building" << "Floor" << "Room" << "Bed Number" << "Type" << "Cost" << "Monthly");
        populate_modRoom_cboxes();

    } else {
        ui->lbl_editUserWarning_3->setText("This vacancy doesn't exist. Please select a valid vacancy.");
    }
}

void MainWindow::on_chk_filter_clicked()
{
    if (ui->chk_filter->isChecked()){
        int nRow = ui->tableWidget_search_client->currentRow();

        QStringList filter = (QStringList() << "*" + ui->tableWidget_search_client->item(nRow, 1)->text() + ", " +
                              ui->tableWidget_search_client->item(nRow, 2)->text() + "*");
        qDebug() << "*" + ui->tableWidget_search_client->item(nRow, 1)->text() + ", " + ui->tableWidget_search_client->item(nRow, 2)->text() + "*";
        populate_tw_caseFiles(filter);
    } else {
        QStringList filter;
        populate_tw_caseFiles(filter);
    }
}

void MainWindow::on_btn_payNew_clicked()
{
    ui->stackedWidget->setCurrentIndex(CLIENTLOOKUP);
    ui->pushButton_processPaymeent->setHidden(false);
}

void MainWindow::on_actionLogout_triggered()
{
    LoginPrompt* w = new LoginPrompt();

    w->show();

    // kill the current thread
    work->cont = false;

    close();
}



//void MainWindow::on_editProgramDrop_currentIndexChanged(const QString &arg1)
//{
//    ui->editUpdate->setEnabled(true);
//}

//void MainWindow::on_comboBox_3_currentTextChanged(const QString &arg1)
//{
//    if (arg1 == "1") {
//        ui->comboBox_4->clear();
//        ui->comboBox_4->addItem("1");
//    } else if (arg1 == "2") {
//        ui->comboBox_4->clear();
//        ui->comboBox_4->addItem("1");
//        ui->comboBox_4->addItem("2");
//    } else if (arg1 == "3") {
//        ui->comboBox_4->clear();
//        ui->comboBox_4->addItem("1");
//        ui->comboBox_4->addItem("2");
//        ui->comboBox_4->addItem("3");
//    } else if (arg1 == "4") {
//        ui->comboBox_4->clear();
//        ui->comboBox_4->addItem("1");
//        ui->comboBox_4->addItem("2");
//        ui->comboBox_4->addItem("3");
//        ui->comboBox_4->addItem("4");
//    } else if (arg1 == "5"){
//        ui->comboBox_4->clear();
//        ui->comboBox_4->addItem("1");
//        ui->comboBox_4->addItem("2");
//        ui->comboBox_4->addItem("3");
//        ui->comboBox_4->addItem("4");
//        ui->comboBox_4->addItem("5");
//    }

//    qDebug() << currentshiftid;
//}

void MainWindow::setShift() {

   QString connName = "leshiftname";
   {
       QSqlDatabase tempDb = QSqlDatabase::database();
       if (dbManager->createDatabase(&tempDb, connName))
       {
           QSqlQuery query(tempDb);

          // qDebug() << "Setting shift now";
           currentshiftid = -1;
           // current time
           QDateTime curtime = QDateTime::currentDateTime();
           QTime curactualtime = QTime::currentTime();
           int dayofweek = curtime.date().dayOfWeek();
           //qDebug() << "today is: " + dayofweek;

           if (dayofweek == 1) {
               query.exec("SELECT * FROM Shift WHERE DayOfWeek='Monday'");

               query.next();
               int numberofshifts = query.value(11).toInt();

               for (int i = 1; i < (numberofshifts + 1); i++) {
                   QTime starttime = query.value((i*2)-1).toTime();
                   QTime endtime = query.value(i*2).toTime();

                   if ((curactualtime < endtime) && (curactualtime >= starttime)) {
                       // change the shiftnum
                       currentshiftid = i;
                       break;
                   }
               }
           } else if (dayofweek == 2) {
               query.exec("SELECT * FROM Shift WHERE DayOfWeek='Tuesday'");

               query.next();
               int numberofshifts = query.value(11).toInt();

               for (int i = 1; i < (numberofshifts + 1); i++) {
                   QTime starttime = query.value((i*2)-1).toTime();
                   QTime endtime = query.value(i*2).toTime();

                   if ((curactualtime < endtime) && (curactualtime >= starttime)) {
                       // change the shiftnum
                       currentshiftid = i;
                       break;
                   }
               }
           } else if (dayofweek == 3) {
               query.exec("SELECT * FROM Shift WHERE DayOfWeek='Wednesday'");

               query.next();
               int numberofshifts = query.value(11).toInt();

               for (int i = 1; i < (numberofshifts + 1); i++) {
                   QTime starttime = query.value((i*2)-1).toTime();
                   QTime endtime = query.value(i*2).toTime();

                   if ((curactualtime < endtime) && (curactualtime >= starttime)) {
                       // change the shiftnum
                       currentshiftid = i;
                       break;
                   }
               }
           } else if (dayofweek == 4) {
               query.exec("SELECT * FROM Shift WHERE DayOfWeek='Thursday'");

               query.next();
               int numberofshifts = query.value(11).toInt();

               for (int i = 1; i < (numberofshifts + 1); i++) {
                   QTime starttime = query.value((i*2)-1).toTime();
                   QTime endtime = query.value(i*2).toTime();

                   if ((curactualtime < endtime) && (curactualtime >= starttime)) {
                       // change the shiftnum
                       currentshiftid = i;
                       break;
                   }
               }
           } else if (dayofweek == 5) {
               query.exec("SELECT * FROM Shift WHERE DayOfWeek='Friday'");

               query.next();
               int numberofshifts = query.value(11).toInt();

               for (int i = 1; i < (numberofshifts + 1); i++) {
                   QTime starttime = query.value((i*2)-1).toTime();
                   QTime endtime = query.value(i*2).toTime();

                   if ((curactualtime < endtime) && (curactualtime >= starttime)) {
                       // change the shiftnum
                       currentshiftid = i;
                       break;
                   }
               }
           } else if (dayofweek == 6) {
               query.exec("SELECT * FROM Shift WHERE DayOfWeek='Saturday'");

               query.next();
               int numberofshifts = query.value(11).toInt();

               for (int i = 1; i < (numberofshifts + 1); i++) {
                   QTime starttime = query.value((i*2)-1).toTime();
                   QTime endtime = query.value(i*2).toTime();

                   if ((curactualtime < endtime) && (curactualtime >= starttime)) {
                       // change the shiftnum
                       currentshiftid = i;
                       break;
                   }
               }
           } else if (dayofweek == 7) {
               query.exec("SELECT * FROM Shift WHERE DayOfWeek='Sunday'");

               query.next();
               int numberofshifts = query.value(11).toInt();

               for (int i = 1; i < (numberofshifts + 1); i++) {
                   QTime starttime = query.value((i*2)-1).toTime();
                   QTime endtime = query.value(i*2).toTime();

                   if ((curactualtime < endtime) && (curactualtime >= starttime)) {
                       // change the shiftnum
                       currentshiftid = i;
                       break;
                   }
               }
           }

//               QSqlQuery query(tempDb);
//               if (!query.exec("SELECT ProfilePic FROM Client WHERE ClientId =" + ClientId))
//               {
//                   qDebug() << "downloadProfilePic query failed";
//                   return false;
//               }
//               query.next();
//               QByteArray data = query.value(0).toByteArray();
//               *img = QImage::fromData(data, "PNG");

       }
       tempDb.close();
   } // Necessary braces: tempDb and query are destroyed because out of scope
   QSqlDatabase::removeDatabase(connName);

//    statusBar()->removeWidget(lbl_curShift);
//    statusBar()->removeWidget(lbl_curUser);
//    statusBar()->addPermanentWidget(lbl_curUser);
//    lbl_curShift = new QLabel();
//    lbl_curShift->setText("Shift Number: " + currentshiftid);
//    statusBar()->addPermanentWidget(lbl_curShift);
      qDebug() << "Shiftno =" << currentshiftid;
}

//void MainWindow::on_btn_saveShift_clicked()
//{
//    QString day = ui->comboBox_2->currentText();
//    int dayindex = ui->comboBox_2->currentIndex();
//    QString shiftno = ui->comboBox_4->currentText();
//    int shiftindex = ui->comboBox_4->currentIndex();

//    QString starttime = ui->timeEdit->text();
//    QString endtime = ui->timeEdit_2->text();

//    // if the shift does not exist, make one
//    QSqlQuery existcheck = dbManager->execQuery("SELECT * FROM Shift WHERE DayOfWeek='" + day + "'");

//    if (!existcheck.next()) {
//        qDebug() << "Doesn't exist";
//        // insert
//        QSqlQuery insert = dbManager->execQuery("INSERT INTO Shift VALUES('" + day + "'"
//                                                ", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, " + ui->comboBox_3->currentText() + ")");
//    }

//    // update
//    QSqlQuery update = dbManager->execQuery("UPDATE Shift SET StartTimeShift" + shiftno +
//                                            "='" + starttime + "' WHERE DayOfWeek = '" + day + "'");
//    QSqlQuery update2 = dbManager->execQuery("UPDATE Shift SET EndTimeShift" + shiftno +
//                                            "='" + endtime + "' WHERE DayOfWeek = '" + day + "'");
//    dbManager->execQuery("UPDATE Shift SET NumShifts=" + ui->comboBox_3->currentText() + " WHERE DayOfWeek = '" + day + "'");

//    on_EditShiftsButton_clicked();
//    ui->comboBox_2->setCurrentIndex(dayindex);
//    ui->comboBox_4->setCurrentIndex(shiftindex);
//}

//void MainWindow::on_comboBox_2_currentTextChanged(const QString &arg1)
//{
//    QSqlQuery existcheck = dbManager->execQuery("SELECT * FROM Shift WHERE DayOfWeek='" + arg1 + "'");

//    if (existcheck.next()) {
//        int numshifts = existcheck.value(11).toInt();
//        ui->comboBox_3->setCurrentIndex(numshifts-1);
//    }
//}

void MainWindow::updatemenuforuser() {
    QSqlQuery roleq = dbManager->execQuery("SELECT Role, EmpName FROM Employee WHERE Username='" + userLoggedIn + "'");

    roleq.next();
    usernameLoggedIn = roleq.value("EmpName").toString();

    qDebug() << "ROLE: " << roleq.value(0).toString();
    qDebug() << "USER: " << userLoggedIn;

    if (roleq.value(0).toString() == "STANDARD") {
        QSizePolicy sp_retain = ui->caseButton->sizePolicy();
        //sp_retain.setRetainSizeWhenHidden(true);
        ui->caseButton->setSizePolicy(sp_retain);
        ui->caseButton->hide();

        QSizePolicy sp_retain2 = ui->caseButton->sizePolicy();
        //sp_retain2.setRetainSizeWhenHidden(true);
        ui->adminButton->setSizePolicy(sp_retain2);
        ui->adminButton->hide();

        //for client delete option(EUNWON)
        ui->button_delete_client->hide();

        currentrole = STANDARD;
    } else if (roleq.value(0).toString() == "CASE WORKER") {
        QSizePolicy sp_retain = ui->caseButton->sizePolicy();
        //sp_retain.setRetainSizeWhenHidden(true);
        ui->adminButton->setSizePolicy(sp_retain);
        ui->adminButton->hide();

        //for client delete option(EUNWON)
        ui->button_delete_client->show();

        currentrole = CASEWORKER;
    } else {
        //for client delete option(EUNWON)
        ui->button_delete_client->show();

        currentrole = ADMIN;
    }

    //display logged in user and current shift in status bar
    lbl_curUser = new QLabel("Logged in as: " + usernameLoggedIn + "  ");
    // lbl_curShift = new QLabel("Shift Number: " + currentshiftid);
    lbl_curUser->setStyleSheet("font-size: 12pt");
    statusBar()->addPermanentWidget(lbl_curUser);
    // statusBar()->addPermanentWidget(lbl_curShift);
}

void MainWindow::on_editRemoveCheque_clicked()
{
    int row = ui->mpTable->selectionModel()->currentIndex().row();
    if(row == -1)
        return;
    QString chequeNo = "";
    QString transId = ui->mpTable->item(row, 6)->text();
    double retAmt = ui->mpTable->item(row, 3)->text().toDouble();
    QString clientId = ui->mpTable->item(row, 5)->text();
    curClient = new Client();
    cNew = true;
    popClientFromId(clientId);
    double curBal = curClient->balance + retAmt;
    if(!dbManager->setPaid(transId, chequeNo )){

    }
    ui->mpTable->removeRow(row);
}


void MainWindow::on_storage_clicked()
{
    ui->stackedWidget->setCurrentIndex(STORAGEPAGE);
    MainWindow::on_storesearch_clicked();
}

void MainWindow::on_storesearch_clicked()
{
    QSqlQuery result;
    result = dbManager->getFullStorage();
    QStringList header, cols;
    header << "Name" << "Date" << "Items" << "" << "";
    cols   << "StorageUserName" << "StorageDate" << "StorageItems" << "ClientId" << "StorageId";
    populateATable(ui->storageTable,header,cols, result, false);
    ui->storageTable->setColumnHidden(2, true);
    ui->storageTable->setColumnHidden(3, true);
    ui->storageTable->setColumnHidden(4, true);

    MainWindow::resizeTableView(ui->storageTable);
}

void MainWindow::on_confirmStorage_clicked()
{
    Storage * store = new Storage(this, curClient->clientId, curClient->fullName, "", "");
    store->exec();

    delete(store);
}

void MainWindow::on_storageEdit_clicked()
{
    int row = ui->storageTable->selectionModel()->currentIndex().row();
    if(row == -1)
        return;
    QString client, storeId, items, name;
    client = ui->storageTable->item(row, 3)->text();
    storeId = ui->storageTable->item(row, 4)->text();
    items = ui->storageTable->item(row, 2)->text();
    name = ui->storageTable->item(row, 0)->text();
    Storage * store = new Storage(this, client, name, items, storeId);
    store->exec();
    delete(store);
    on_storesearch_clicked();
}

void MainWindow::on_storageTable_itemClicked(QTableWidgetItem *item)
{
    int row = item->row();
    QString items = ui->storageTable->item(row, 2)->text();
    ui->storageInfo->clear();
    ui->storageInfo->setEnabled(false);
    ui->storageInfo->insertPlainText(items);
}

void MainWindow::on_storageDelete_clicked()
{
    int row = ui->storageTable->selectionModel()->currentIndex().row();
    if(row == -1)
        return;
    if(!doMessageBox("Are you sure you want to delete?"))
        return;
    QString storeId;
    storeId = ui->storageTable->item(row, 4)->text();
    if(!dbManager->removeStorage(storeId))
        qDebug() << "Error removing storage";
    ui->storageTable->removeRow(row);
}




void MainWindow::on_addStorageClient_clicked()
{
    QSqlQuery result;
    if(ui->tableWidget_search_client->selectionModel()->currentIndex().row() == -1)
        return;
    result = dbManager->pullClient(curClientID);
    if(!result.next())
        return;
    QString name = result.value("LastName").toString() + ", " + result.value("FirstName").toString();

    Storage * sto = new Storage(this, curClientID, name, "", "");
    sto->exec();
    delete(sto);
    setStorageClient();
}


void MainWindow::on_editDelete_clicked()
{
    QSqlQuery role = dbManager->getRole(userLoggedIn);
    if(!role.next())
        return;
    QString userRole = role.value("Role").toString();
    if(userRole != "ADMIN"){
        doMessageBox("Admin Only Feature");
        return;
    }
    if(!doMessageBox("Deleting is permenant, booking cost is refunded. Continue?"))
        return;
    int row = ui->editLookupTable->selectionModel()->currentIndex().row();
    if(row == -1)
        return;
    ui->editDelete->setEnabled(false);
    curBook = new Booking();
    bNew = true;
    popBookFromRow();
    QSqlQuery result = dbManager->getBalance(curBook->clientId);
    if(!result.next()){
        return;
    }
    double curBal = result.value("Balance").toString().toDouble();
    double bCost = dbManager->getBookingCost(curBook->bookID);
    curBal += bCost;

    if(!dbManager->updateBalance(curBal, curBook->clientId))
            qDebug() << "Error updating balance";

    if(!dbManager->deleteBooking(curBook->bookID, usernameLoggedIn, currentshiftid)){
        qDebug() << "Error deleting booking";
    }
    if(!dbManager->removeLunchesMulti(curBook->startDate, curBook->clientId))
        qDebug() << "Error remiving lunches";
    if(!dbManager->deleteWakeupsMulti(curBook->startDate, curBook->clientId))
        qDebug() << "Error removing wakeups";
    ui->editLookupTable->removeRow(row);
    ui->editDelete->setEnabled(true);
}


double MainWindow::quickCost(std::pair<int,int> p, double daily, double monthly){
    int days, months;
    double dailyCost, totalCost;
    days = p.second;
    months = p.first;
    dailyCost = days * daily;
    if(dailyCost > monthly){
        dailyCost = monthly;
    }

    totalCost = monthly * months + dailyCost;
    return totalCost;


}

double MainWindow::realCost(QDate start, QDate end, double daily, double monthly){
    int days, months, totalDays;
    double dailyCost, totalCost;
    months = 0;
    totalDays = end.toJulianDay() - start.toJulianDay();
    QDate holdEnd;
    holdEnd = end;
    while((holdEnd = holdEnd.addMonths(-1)) >= start)
        months++;
    holdEnd = holdEnd.addMonths(1);
    if(holdEnd == start)
        days = 0;
    else{
        holdEnd = start.addMonths(months);
        days = end.toJulianDay() - holdEnd.toJulianDay();
    }
    dailyCost = days * daily;
    if(dailyCost > monthly){
        dailyCost = monthly;
    }

    totalCost = monthly * months + dailyCost;
    return totalCost;

}
std::pair<int,int> MainWindow::monthDay(QDate start, QDate end){
    int days, months, totalDays;
    months = 0;
    totalDays = end.toJulianDay() - start.toJulianDay();
    QDate holdEnd;
    holdEnd = end;
    while((holdEnd = holdEnd.addMonths(-1)) >= start)
        months++;
    holdEnd = holdEnd.addMonths(1);
    if(holdEnd == start)
        days = 0;
    else{
        holdEnd = start.addMonths(months);
        days = end.toJulianDay() - holdEnd.toJulianDay();
    }
    std::pair<int,int> p = std::make_pair(months, days);
    return p;
}

void MainWindow::on_addMonth_clicked()
{

    QDate switcher, possibleChange;
    switcher = ui->endDateEdit->date();
    possibleChange = switcher.addDays(-1);
    if(possibleChange == ui->startDateEdit->date())
        switcher = switcher.addDays(-1);
    switcher = switcher.addMonths(1);
    ui->endDateEdit->setDate(switcher);
}

void MainWindow::on_bookingTable_itemClicked(QTableWidgetItem *item)
{
    int row = item->row();
    if(row == -1)
        return;
    double daily, monthly;
    daily = ui->bookingTable->item(row, 3)->text().toDouble();
    monthly = ui->bookingTable->item(row, 4)->text().toDouble();
    std::pair<int,int> p = std::make_pair(ui->bookMonthLabel->text().toInt(), ui->bookDayLabel->text().toInt());
    double cost = quickCost(p,daily, monthly);
    ui->bookCostLabel->setText(QString::number(cost, 'f',2));
}

//void MainWindow::on_shiftNum_currentIndexChanged(int index)
//{
//    lockupShifts();
//    showShifts(index + 1);
//}

//void MainWindow::on_shiftDay_currentIndexChanged(int index)
//{
//    lockupShifts();

//}
//void MainWindow::lockupShifts(){
//    QTime midnight = QTime::fromString("00:00:00", "hh:mm:ss");
//    ui->shiftS1->setTime(midnight);
//    midnight = midnight.addSecs(1);
//    ui->shiftS2->setTime(midnight);
//    midnight = midnight.addSecs(1);
//    ui->shiftS3->setTime(midnight);
//    midnight = midnight.addSecs(1);
//    ui->shiftS4->setTime(midnight);
//    midnight = midnight.addSecs(1);
//    ui->shiftS5->setTime(midnight);
//    midnight = midnight.addSecs(1);
//    ui->shiftE1->setTime(midnight);
//    midnight = midnight.addSecs(1);
//    ui->shiftE2->setTime(midnight);
//    midnight = midnight.addSecs(1);
//    ui->shiftE3->setTime(midnight);
//    midnight = midnight.addSecs(1);
//    ui->shiftE4->setTime(midnight);
//    midnight = midnight.addSecs(1);
//    ui->shiftE5->setTime(midnight);

//}
//void MainWindow::showShifts(int num){
//    ui->shiftS1->setHidden(true);
//    ui->shiftE1->setHidden(true);
//    ui->shiftS2->setHidden(true);
//    ui->shiftE2->setHidden(true);
//    ui->shiftS3->setHidden(true);
//    ui->shiftE3->setHidden(true);
//    ui->shiftS4->setHidden(true);
//    ui->shiftE4->setHidden(true);
//    ui->shiftS5->setHidden(true);
//    ui->shiftE5->setHidden(true);
//    ui->shiftS1->setEnabled(false);
//    ui->shiftS2->setEnabled(false);
//    ui->shiftS3->setEnabled(false);
//    ui->shiftS4->setEnabled(false);
//    ui->shiftS5->setEnabled(false);
//    ui->shiftE1->setEnabled(false);
//    ui->shiftE2->setEnabled(false);
//    ui->shiftE3->setEnabled(false);
//    ui->shiftE4->setEnabled(false);
//    ui->shiftE5->setEnabled(false);
//    numShift = num;
//    QTime endTime = QTime::fromString("23:59:59", "hh:mm:ss");
//    if(num-- > 0){
//        ui->shiftS1->setHidden(false);
//        ui->shiftE1->setEnabled(true);
//        ui->shiftE1->setHidden(false);
//        if(!num){
//            ui->shiftE1->setTime(endTime);
//            ui->shiftE1->setEnabled(false);
//        }
//    }
//    if(num-- > 0){
//        ui->shiftS2->setHidden(false);
//        ui->shiftE2->setHidden(false);
//        ui->shiftE2->setEnabled(true);

//        if(!num){
//            ui->shiftE2->setTime(endTime);
//            ui->shiftE2->setEnabled(false);
//        }
//    }
//    if(num-- > 0){
//        ui->shiftS3->setHidden(false);
//        ui->shiftE3->setHidden(false);
//        ui->shiftE3->setEnabled(true);

//        if(!num){
//            ui->shiftE3->setTime(endTime);
//            ui->shiftE3->setEnabled(false);
//        }
//    }
//    if(num-- > 0){
//        ui->shiftS4->setHidden(false);
//        ui->shiftE4->setHidden(false);
//        ui->shiftE4->setEnabled(true);

//        if(!num){
//            ui->shiftE4->setTime(endTime);
//            ui->shiftE4->setEnabled(false);
//        }
//    }
//    if(num-- > 0){
//        ui->shiftS5->setHidden(false);
//        ui->shiftE5->setHidden(false);
//        ui->shiftE5->setEnabled(true);

//        if(!num){
//            ui->shiftE5->setTime(endTime);
//            ui->shiftE5->setEnabled(false);
//        }
//    }

//}

//void MainWindow::on_shiftE1_timeChanged(const QTime &time)
//{
//    if(numShift <= 1){
//        return;
//    }
//    if(time <= ui->shiftS1->time()){
//        ui->shiftE1->setTime(ui->shiftS1->time().addSecs(60));
//        return;
//    }
//    ui->shiftS2->setTime(time.addSecs(1));
//}

//void MainWindow::on_shiftS2_timeChanged(const QTime &time)
//{
//    if(time <= ui->shiftE1->time()){
//        ui->shiftS2->setTime(ui->shiftE1->time().addSecs(60));
//        return;
//    }
//    if(ui->shiftE2->time() < time)
//        ui->shiftE2->setTime(time.addSecs(1));
//}

//void MainWindow::on_shiftE2_timeChanged(const QTime &time)
//{
//    if(numShift <= 2){
//        return;
//    }
//    if(time <= ui->shiftS2->time()){
//        ui->shiftE2->setTime(ui->shiftS2->time().addSecs(60));
//        return;
//    }
//    ui->shiftS3->setTime(time.addSecs(1));
//}

//void MainWindow::on_shiftS3_timeChanged(const QTime &time)
//{

//    if(time <= ui->shiftE2->time()){
//        ui->shiftS3->setTime(ui->shiftE2->time().addSecs(60));
//        return;
//    }
//    if(ui->shiftE3->time() < time)
//         ui->shiftE3->setTime(time.addSecs(60));
//}

//void MainWindow::on_shiftS4_timeChanged(const QTime &time)
//{

//    if(time <= ui->shiftE3->time()){
//        ui->shiftS4->setTime(ui->shiftE3->time().addSecs(60));
//        return;
//    }
//    if(ui->shiftE4->time() < time)
//        ui->shiftE4->setTime(time.addSecs(60));
//}

//void MainWindow::on_shiftS5_timeChanged(const QTime &time)
//{
//    if(time <= ui->shiftE4->time()){
//        ui->shiftS5->setTime(ui->shiftE4->time().addSecs(60));
//        return;
//    }
//    if(time >= ui->shiftE5->time()){
//        ui->shiftS5->setTime(ui->shiftE5->time().addSecs(-60));
//        return;
//    }
//}

//void MainWindow::on_shiftE3_timeChanged(const QTime &time)
//{
//    if(numShift <=3){
//        return;
//    }
//    if(time <= ui->shiftS3->time()){
//        ui->shiftE3->setTime(ui->shiftS3->time().addSecs(60));
//        return;
//    }
//    ui->shiftS4->setTime(time.addSecs(60));
//}

//void MainWindow::on_shiftE4_timeChanged(const QTime &time)
//{
//    if(numShift <= 4){
//        return;
//    }
//    if(time <= ui->shiftS4->time()){
//        ui->shiftE4->setTime(ui->shiftS4->time().addSecs(60));
//        return;
//    }
//    ui->shiftS5->setTime(time.addSecs(60));
//}

void MainWindow::on_editCost_textChanged(const QString &arg1)
{
    if(dateChanger){
        dateChanger = false;
        return;
    }
    ui->editUpdate->setEnabled(true);
    if(!checkNumber(arg1))
        return;
    double newCost = arg1.toDouble();
    double refund = ui->editCancel->text().toDouble();
    double origCost = ui->editOC->text().toDouble();

    if(newCost < origCost){
        ui->editRefundLabel->setText("Refund");
        double realRefund = newCost - origCost + refund;

        if(realRefund > 0)
            realRefund = 0;
        realRefund = realRefund * -1;
        ui->editRefundAmt->setText(QString::number(realRefund, 'f', 2));
    }
    else{
        ui->editRefundLabel->setText("Owed");
        ui->editRefundAmt->setText(QString::number(newCost - origCost, 'f', 2));
    }
}
void MainWindow::on_shiftReport_tabWidget_currentChanged(int index)
{
    switch (index)
    {
        case 0:
            resizeTableView(ui->booking_tableView);
            break;
        case 1:
            resizeTableView(ui->transaction_tableView);
            break;
        case 2:
            resizeTableView(ui->clientLog_tableView);
            break;
        case 3:
            resizeTableView(ui->other_tableView);
    }
}

void MainWindow::on_dailyReport_tabWidget_currentChanged(int index)
{
    switch (index)
    {
        case 0:
            resizeTableView(ui->checkout_tableView);
            break;
        case 1:
            resizeTableView(ui->vacancy_tableView);
            break;
        case 2:
            resizeTableView(ui->lunch_tableView);
            break;
        case 3:
            resizeTableView(ui->wakeup_tableView);
    }
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox aboutBox;
    aboutBox.setText("ARCWay Version " + versionNo +"\n\n"
                                                    "Colin Bose\ncolin.bose2@gmail.com\n\n"
                                                    "Dennis Chau\ndennis.chau@outlook.com\n\n"
                                                    "Eunwon Moon\nimoongom@gmail.com\n\n"
                                                    "Hank Lo\nhlo1453@gmail.com\n\n"
                                                    "Joseph Tam\njosephtam3@gmail.com\n\n"
                                                    "BCIT 2017");
    aboutBox.setIcon(QMessageBox::Information);
    aboutBox.exec();
    qDebug() << "about clicked";
}

void MainWindow::on_editClient_returnPressed()
{
    MainWindow::on_editSearch_clicked();
}



/*============================================================
 * SHIFT CHANGE
 * ===========================================================*/
void MainWindow::on_shift_dayOpt_currentIndexChanged(const QString &arg1)
{
    shiftExist = false;
    selectedDayIdx = ui->shift_dayOpt->currentIndex();
    if(arg1 ==""){
        ui->shift_num->setEnabled(false);
        ui->pushButton_shift_save->setEnabled(false);
        selectedDay ="";
        return;
    }
    ui->shift_num->setEnabled(true);
    ui->pushButton_shift_save->setEnabled(true);
    ui->shift_num->setCurrentIndex(0);
    showAllShiftEdit(true);
//    qDebug()<<"shifDay CurrentIndexChaged" << arg1;
    selectedDay = arg1;
    shiftSize = 0;

//    qDebug()<<"SELECTED DAY"<<selectedDay<<selectedDayIdx;
    readShiftDb(selectedDay);



}

void MainWindow::readShiftDb(QString day){
//    qDebug()<<"READ shift DB" << day;
    QSqlQuery dailyShift = dbManager->getShiftInfoDaily(day);
    while(dailyShift.next()){
        shiftSize = dailyShift.value("NumShifts").toInt();
        ui->shift_num->setCurrentIndex(shiftSize);
        setShiftTimeDialog(false);
        shiftExist = true;
        switch(shiftSize){
            case 5:
                ui->shift5_S->setTime(dailyShift.value("StartTimeShift5").toTime());
                ui->shift5_E->setTime(dailyShift.value("EndTimeShift5").toTime());
            case 4:
                ui->shift4_S->setTime(dailyShift.value("StartTimeShift4").toTime());
                ui->shift4_E->setTime(dailyShift.value("EndTimeShift4").toTime());
            case 3:
                ui->shift3_S->setTime(dailyShift.value("StartTimeShift3").toTime());
                ui->shift3_E->setTime(dailyShift.value("EndTimeShift3").toTime());
            case 2:
                ui->shift2_S->setTime(dailyShift.value("StartTimeShift2").toTime());
                ui->shift2_E->setTime(dailyShift.value("EndTimeShift2").toTime());
            case 1:

                ui->shift1_S->setTime(dailyShift.value("StartTimeShift1").toTime());
                ui->shift1_E->setTime(dailyShift.value("EndTimeShift1").toTime());
        }
    }
    qDebug()<<" DB READ DONE";

}

void MainWindow::on_shift_num_currentIndexChanged(int index)
{
    shiftSize = index;
    setShiftTimeDialog(true);
}


void MainWindow::setShiftTimeDialog(bool resetTime){
    QTime startTime, endTime;
    double TimeGap = (double)24/(shiftSize);
    startTime.setHMS(TimeGap,0,0);
    endTime.setHMS(TimeGap-1,59,0);
    initTime();
    showAllShiftEdit(false);


    switch(shiftSize){
        case 0:  //no selection
            ui->label_Shift_Index->hide();
            ui->label_Shift_Start->hide();
            ui->label_Shift_End->hide();
            ui->checkBox_shift_auto_endtime->hide();
            ui->label_Shift1->hide();
            ui->shift1_S->hide();
            ui->shift1_E->hide();
        case 1:  //select 1
            ui->label_Shift2->hide();
            ui->shift2_S->hide();
            ui->shift2_E->hide();

        case 2: //select 2
            ui->label_Shift3->hide();
            ui->shift3_S->hide();
            ui->shift3_E->hide();

        case 3:
            ui->label_Shift4->hide();
            ui->shift4_S->hide();
            ui->shift4_E->hide();

        case 4:
            ui->label_Shift5->hide();
            ui->shift5_S->hide();
            ui->shift5_E->hide();
    }

    if(!resetTime)
        return;

    //change time automatically
    for(int i = 1; i < shiftSize; i++){
            startTime.setHMS(TimeGap*i,0,0);
            endTime.setHMS(TimeGap*i-1,59,0);

        switch(i){
            case 1:
                ui->shift1_E->setTime(endTime);
                ui->shift2_S->setTime(startTime);
                break;
            case 2:
                ui->shift3_S->setTime(startTime);
                ui->shift2_E->setTime(endTime);
                break;
            case 3:
                ui->shift4_S->setTime(startTime);
                ui->shift3_E->setTime(endTime);
                break;
            case 4:
                ui->shift5_S->setTime(startTime);
                ui->shift4_E->setTime(endTime);
                break;
        }
    }
}

void MainWindow::showAllShiftEdit(bool display)
{

    qDebug()<<"SHOW SHIFT EDIT";

        ui->label_Shift_Index->setHidden(display);
        ui->label_Shift_Start->setHidden(display);
        ui->label_Shift_End->setHidden(display);
        ui->checkBox_shift_auto_endtime->setHidden(display);
        ui->label_Shift1->setHidden(display);
        ui->shift1_S->setHidden(display);
        ui->shift1_E->setHidden(display);

        ui->label_Shift2->setHidden(display);
        ui->shift2_S->setHidden(display);
        ui->shift2_E->setHidden(display);
        ui->label_Shift3->setHidden(display);
        ui->shift3_S->setHidden(display);
        ui->shift3_E->setHidden(display);
        ui->label_Shift4->setHidden(display);
        ui->shift4_S->setHidden(display);
        ui->shift4_E->setHidden(display);
        ui->label_Shift5->setHidden(display);
        ui->shift5_S->setHidden(display);
        ui->shift5_E->setHidden(display);
}

void MainWindow::initTime(){
    QTime timeSet;
    timeSet.setHMS(0,0,0);
    ui->shift1_S->setTime(timeSet);
}


void MainWindow::changeTimeSet(QTimeEdit* endUI, QTime time){
    QTime temp;
    if(time.minute()== 0)
        temp.setHMS((time.hour()-1), time.minute()+59,59);
    else
        temp.setHMS((time.hour()), time.minute()-1,59);
    qDebug()<<"CURRENT TIME " <<time.toString()<< temp.toString();
    endUI->setTime(temp);
}


void MainWindow::on_shift1_S_editingFinished()
{
    if(!ui->checkBox_shift_auto_endtime->isChecked())
        return;
    QTime time = ui->shift1_S->time();
    time.setHMS(time.hour()+1, 0, 0);
    changeTimeSet(ui->shift1_E, time);
    time = time.addSecs(60);
    changeTimeSet(ui->shift2_S, time);

}
void MainWindow::on_shift2_S_editingFinished()
{
    if(!ui->checkBox_shift_auto_endtime->isChecked())
        return;
    QTime time = ui->shift2_S->time();
    changeTimeSet(ui->shift1_E, time);
}

void MainWindow::on_shift3_S_editingFinished()
{
    if(!ui->checkBox_shift_auto_endtime->isChecked())
        return;
    QTime time = ui->shift3_S->time();
    changeTimeSet(ui->shift2_E, time);

}

void MainWindow::on_shift4_S_editingFinished()
{
    if(!ui->checkBox_shift_auto_endtime->isChecked())
        return;
    QTime time = ui->shift4_S->time();
    changeTimeSet(ui->shift3_E, time);

}

void MainWindow::on_shift5_S_editingFinished()
{
    if(!ui->checkBox_shift_auto_endtime->isChecked())
        return;
    QTime time = ui->shift5_S->time();
    changeTimeSet(ui->shift4_E, time);
}

void MainWindow::changeUI(){
    qDebug()<<"UI update" << sun.empty();
    int cnt;
    if(!sun.empty()){
        qDebug()<<"sunDAY not empty";
        shiftReportInit(SUN);
        cnt = sun.size();
        switch(cnt){
        case 5:
            ui->label_shift_sun5->show();
            ui->label_shift_sun5->setText(sun.at(4).at(0));

        case 4:
            ui->label_shift_sun4->show();
            ui->label_shift_sun4->setText(sun.at(3).at(0));

        case 3:
            ui->label_shift_sun3->show();
            ui->label_shift_sun3->setText(sun.at(2).at(0));

        case 2:
            ui->label_shift_sun2->show();
            ui->label_shift_sun2->setText(sun.at(1).at(0));

        case 1:
            ui->label_shift_sun1->show();
            ui->label_shift_sun1->setText(sun.at(0).at(0));
        }

    }
    if(!mon.empty()){
        qDebug()<<"MONDAY not empty";
        shiftReportInit(MON);
        switch(mon.size()){
        case 5:
            ui->label_shift_mon5->show();
            ui->label_shift_mon5->setText(mon.at(4).at(0));

        case 4:
            ui->label_shift_mon4->show();
            ui->label_shift_mon4->setText(mon.at(3).at(0));

        case 3:
            ui->label_shift_mon3->show();
            ui->label_shift_mon3->setText(mon.at(2).at(0));

        case 2:
            ui->label_shift_mon2->show();
            ui->label_shift_mon2->setText(mon.at(1).at(0));

        case 1:
            ui->label_shift_mon1->show();
            ui->label_shift_mon1->setText(mon.at(0).at(0));
        }
    }
    if(!tue.empty()){
        shiftReportInit(TUE);
        switch(tue.size()){
        case 5:
            ui->label_shift_tue5->show();
            ui->label_shift_tue5->setText(tue.at(4).at(0));

        case 4:
            ui->label_shift_tue4->show();
            ui->label_shift_tue4->setText(tue.at(3).at(0));

        case 3:
            ui->label_shift_tue3->show();
            ui->label_shift_tue3->setText(tue.at(2).at(0));

        case 2:
            ui->label_shift_tue2->show();
            ui->label_shift_tue2->setText(tue.at(1).at(0));

        case 1:
            ui->label_shift_tue1->show();
            ui->label_shift_tue1->setText(tue.at(0).at(0));
        }
    }

    if(!wed.empty()){
        shiftReportInit(WED);
        switch(wed.size()){
        case 5:
            ui->label_shift_wed5->show();
            ui->label_shift_wed5->setText(wed.at(4).at(0));

        case 4:
            ui->label_shift_wed4->show();
            ui->label_shift_wed4->setText(wed.at(3).at(0));

        case 3:
            ui->label_shift_wed3->show();
            ui->label_shift_wed3->setText(wed.at(2).at(0));

        case 2:
            ui->label_shift_wed2->show();
            ui->label_shift_wed2->setText(wed.at(1).at(0));

        case 1:
            ui->label_shift_wed1->show();
            ui->label_shift_wed1->setText(wed.at(0).at(0));
        }
    }

    if(!thur.empty()){
        shiftReportInit(THUR);
        switch(thur.size()){
        case 5:
            ui->label_shift_thur5->show();
            ui->label_shift_thur5->setText(thur.at(4).at(0));

        case 4:
            ui->label_shift_thur4->show();
            ui->label_shift_thur4->setText(thur.at(3).at(0));

        case 3:
            ui->label_shift_thur3->show();
            ui->label_shift_thur3->setText(thur.at(2).at(0));

        case 2:
            ui->label_shift_thur2->show();
            ui->label_shift_thur2->setText(thur.at(1).at(0));

        case 1:
            ui->label_shift_thur1->show();
            ui->label_shift_thur1->setText(thur.at(0).at(0));
        }

    }

    if(!fri.empty()){
        qDebug()<<"FRIDAY not empty";
        shiftReportInit(FRI);
        switch(fri.size()){
        case 5:
            ui->label_shift_fri5->show();
            ui->label_shift_fri5->setText(fri.at(4).at(0));

        case 4:
            ui->label_shift_fri4->show();
            ui->label_shift_fri4->setText(fri.at(3).at(0));

        case 3:
            ui->label_shift_fri3->show();
            ui->label_shift_fri3->setText(fri.at(2).at(0));

        case 2:
            ui->label_shift_fri2->show();
            ui->label_shift_fri2->setText(fri.at(1).at(0));

        case 1:
            ui->label_shift_fri1->show();
            ui->label_shift_fri1->setText(fri.at(0).at(0));
        }
    }

    if(!sat.empty()){
        qDebug()<<"SATDAY not empty";
        shiftReportInit(SAT);
        switch(sat.size()){
        case 5:
            ui->label_shift_sat5->show();
            ui->label_shift_sat5->setText(sat.at(4).at(0));

        case 4:
            ui->label_shift_sat4->show();
            ui->label_shift_sat4->setText(sat.at(3).at(0));

        case 3:
            ui->label_shift_sat3->show();
            ui->label_shift_sat3->setText(sat.at(2).at(0));

        case 2:
            ui->label_shift_sat2->show();
            ui->label_shift_sat2->setText(sat.at(1).at(0));

        case 1:
            ui->label_shift_sat1->show();
            ui->label_shift_sat1->setText(sat.at(0).at(0));
        }

    }

}
//initialize shift summary label
void MainWindow::shiftReportInit(int dayType){
    switch(dayType){
        case SUN:
            ui->label_shift_sun1->hide();
            ui->label_shift_sun2->hide();
            ui->label_shift_sun3->hide();
            ui->label_shift_sun4->hide();
            ui->label_shift_sun5->hide();
            break;
        case MON:
            ui->label_shift_mon1->hide();
            ui->label_shift_mon2->hide();
            ui->label_shift_mon3->hide();
            ui->label_shift_mon4->hide();
            ui->label_shift_mon5->hide();
            break;
        case TUE:
            ui->label_shift_tue1->hide();
            ui->label_shift_tue2->hide();
            ui->label_shift_tue3->hide();
            ui->label_shift_tue4->hide();
            ui->label_shift_tue5->hide();
            break;
        case WED:
            ui->label_shift_wed1->hide();
            ui->label_shift_wed2->hide();
            ui->label_shift_wed3->hide();
            ui->label_shift_wed4->hide();
            ui->label_shift_wed5->hide();
            break;
        case THUR:
            ui->label_shift_thur1->hide();
            ui->label_shift_thur2->hide();
            ui->label_shift_thur3->hide();
            ui->label_shift_thur4->hide();
            ui->label_shift_thur5->hide();
            break;
        case FRI:
            ui->label_shift_fri1->hide();
            ui->label_shift_fri2->hide();
            ui->label_shift_fri3->hide();
            ui->label_shift_fri4->hide();
            ui->label_shift_fri5->hide();
            break;
        case SAT:
            ui->label_shift_sat1->hide();
            ui->label_shift_sat2->hide();
            ui->label_shift_sat3->hide();
            ui->label_shift_sat4->hide();
            ui->label_shift_sat5->hide();
            break;
    }
}

//SAVE client info
void MainWindow::on_pushButton_shift_save_clicked()
{
    EditShiftInfo();
    ReadCurrentShift(selectedDay);
}

void MainWindow::EditShiftInfo(){
    QStringList shiftValueList;
    getShiftList(&shiftValueList);
    if(!dbManager->updateShift(shiftExist, selectedDay, &shiftValueList))
    {
        statusBar()->showMessage(selectedDay + " NOT CHANGED. TRY AGAIN.", 10000);
        return;
    }
    shiftExist = true;
    qDebug()<<"SHIFT UPDATE SUCCESS";
    statusBar()->showMessage(selectedDay + " SHIFT UPDATE SUCCESS", 10000);

}

//read shift information
void MainWindow::ReadCurrentShift(QString readDay){
    QSqlQuery readShiftQ = dbManager->getShiftInfoDaily(readDay);
    while(readShiftQ.next()){
        QString dayTag = readShiftQ.value("DayOfWeek").toString();
        qDebug()<<"DAY IS " << dayTag;
        selectedDayIdx = ui->shift_dayOpt->findData(dayTag);
        if(dayTag == "Sunday")
            updateList(&sun,readShiftQ);
        else if(dayTag == "Monday")
            updateList(&mon,readShiftQ);
        else if(dayTag == "Tuesday")
            updateList(&tue,readShiftQ);
        else if(dayTag == "Wednesday")
            updateList(&wed,readShiftQ);
        else if(dayTag == "Thursday")
            updateList(&thur,readShiftQ);
        else if(dayTag == "Friday")
            updateList(&fri,readShiftQ);
        else if(dayTag == "Saturday")
            updateList(&sat,readShiftQ);
    }
    changeUI();

}

// get shift time to update database
void MainWindow::getShiftList(QStringList *shiftList){
    qDebug()<<"get shift list. shift Size "<<shiftSize;
    if(!shiftExist)
        *shiftList<<selectedDay;
    if(shiftSize>=1 )
    {
        *shiftList << ui->shift1_S->time().toString()
                   << ui->shift1_E->time().toString();
    }

    if(shiftSize >= 2){
        *shiftList << ui->shift2_S->time().toString()
                   << ui->shift2_E->time().toString();
    }

    if(shiftSize >= 3 ){
        *shiftList << ui->shift3_S->time().toString()
                   << ui->shift3_E->time().toString();
    }

    if(shiftSize >= 4 ){
        *shiftList << ui->shift4_S->time().toString()
                   << ui->shift4_E->time().toString();
    }

    if(shiftSize >= 5){
        *shiftList << ui->shift5_S->time().toString()
                   << ui->shift5_E->time().toString();
    }

    //left of empty slots
    for(int i = shiftSize; i < 5; i++)
    {
        *shiftList <<""<<"";
    }
    *shiftList << QString::number(shiftSize);

}

//update using query
void MainWindow::updateList(QVector<QStringList>* day, QSqlQuery infoQuery){
    if(day->size() != 0)
        day->clear();
    QStringList tempList;

    shiftSize = infoQuery.value("NumShifts").toInt();
    switch(shiftSize){
    case 5:
        tempList << infoQuery.value(9).toTime().toString("h:mmAP")
                 << infoQuery.value(10).toTime().toString("h:mmAP");
        day->push_front(tempList);

    case 4:
        tempList.clear();
        tempList<< infoQuery.value(7).toTime().toString("h:mmAP")
                << infoQuery.value(8).toTime().toString("h:mmAP");
        day->push_front(tempList);

    case 3:
        tempList.clear();
        tempList << infoQuery.value(5).toTime().toString("h:mmAP")
                << infoQuery.value(6).toTime().toString("h:mmAP");
        day->push_front(tempList);

    case 2:
        tempList.clear();
        tempList<< infoQuery.value(3).toTime().toString("h:mmAP")
                << infoQuery.value(4).toTime().toString("h:mmAP");
        day->push_front(tempList);

    case 1:
        tempList.clear();
        tempList<< infoQuery.value(1).toTime().toString("h:mmAP")
                << infoQuery.value(2).toTime().toString("h:mmAP");
        day->push_front(tempList);

    }

}

//WILLBE DELETED AFTER TESTING
void MainWindow::on_pushButton_newFeature_clicked()
{
    ui->stackedWidget->setCurrentIndex(18);
}


void MainWindow::on_checkBox_shift_auto_endtime_clicked(bool checked)
{
    ui->shift1_E->setDisabled(checked);
    ui->shift2_E->setDisabled(checked);
    ui->shift3_E->setDisabled(checked);
    ui->shift4_E->setDisabled(checked);
    ui->shift5_E->setDisabled(checked);
}
/*---------------------------------------------------------------------------
 * EDIT SHIFT FINISHED(EUNWON)
 *============================================================================*/




void MainWindow::on_btnViewTranns_clicked()
{
    int index = ui->cbox_payDateRange->currentIndex();
    MainWindow::on_cbox_payDateRange_activated(index);
}

void MainWindow::addCurrencyToTableWidget(QTableWidget* table, int col){
    int numRows = table->rowCount();
    for (int row = 0; row < numRows; ++row) {
        qDebug() << "modifying item: " << table->item(row, col)->text();
        QString value = QString::number(table->item(row, col)->text().toFloat(), 'f', 2);
        //QString value = table->item(row, col)->text();
        table->setItem(row, col, new QTableWidgetItem("$"+value));
    }
}

void MainWindow::createTextReceipt(QString totalCost, QString payType, QString payTotal,
                                   QString start, QString end, QString length, bool stay, bool refund){
    QString timestamp = QDate::currentDate().toString("yyyyMMdd") + QTime::currentTime().toString("hhmmss");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                               "/receipt_" + timestamp + ".txt",
                               tr("Text File (*.txt)"));
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        statusBar()->showMessage(tr("Receipt file save failed. Please try saving again or saving to another directory."), 4000);
        return;
    }


    QTextStream out(&file);
    out << getOrgName()+"\n" +
           getStreetNo() + " " +getStreetName() + "\n" +
           getCity()+ " " +getProvince()+ " " +getZip()+"\n"+
           getPhone()+"\n"+
           getWebsite()+"\n"+
           "**********************************\n"
           "\n"
           "\n";
           if (stay){
               out << "Stay                     $"+totalCost+"\n"
               "" + start + " - " + end + "\n"
               "" + length +" nights\n";
            } else {
               out << "\n\n\n";
           }
           out << "\n"
           "\n"
           "\n"
           "\n"
           "_________________________________\n";
           if (refund) {
               out << "Refund:                       Yes\n";
           }
           out << "Payment Type: " + payType + "\n"
           "Payment Total:           $"+payTotal+"\n"
           "\n"
           "\n"
           "**********************************\n"
           "Date: " + QDate::currentDate().toString("yyyy-MM-dd") + " Time: " + QTime::currentTime().toString("H:mm AP") + "\n"
           "Receipt No: "+timestamp+"\n"
           "\n"
           "\n"
           "Thank you";
           statusBar()->showMessage(tr("Receipt file saved!"), 4000);
}

void MainWindow::on_actionReceipt_triggered()
{

    switch(ui->stackedWidget->currentIndex()) {
    case CONFIRMBOOKING:
        createTextReceipt(ui->confirmCost->text(), transType, ui->confirmTotalPaid->text(), curBook->stringStart,
                          curBook->stringEnd, ui->confirmLength->text(), true, isRefund);
        break;
    case CLIENTLOOKUP:
        createTextReceipt(QString::number(trans->amount),trans->type,QString::number(trans->paidToday),
                          QString(),QString(),QString(),false,isRefund);
        break;
    }
}

void MainWindow::addCurrencyNoSignToTableWidget(QTableWidget* table, int col){
    int numRows = table->rowCount();
    for (int row = 0; row < numRows; ++row) {
        QString value = QString::number(table->item(row, col)->text().toFloat(), 'f', 2);
        //QString value = table->item(row, col)->text();
        table->setItem(row, col, new QTableWidgetItem(value));
    }
}

void MainWindow::on_EditAddressButton_clicked()
{
    addHistory(ADMINPAGE);
    ui->stackedWidget->setCurrentIndex(EDITADDRESS);

    QSettings settings(QSettings::IniFormat, QSettings::SystemScope,
                       "The Salvation Army", "ARCWay");

    settings.beginGroup("Address");

    ui->le_orgName->setText(settings.value("orgName").toString());
    ui->le_streetNo->setText(settings.value("streetNumber").toString());
    ui->le_streetName->setText(settings.value("streetName").toString());
    ui->le_city->setText(settings.value("city").toString());
    ui->le_province->setText(settings.value("province").toString());
    ui->le_zip->setText(settings.value("zip").toString());
    ui->le_phone->setText(settings.value("phone").toString());
    ui->le_website->setText(settings.value("website").toString());

    settings.endGroup();
}

void MainWindow::on_btn_saveAd_clicked()
{
    QSettings settings(QSettings::IniFormat, QSettings::SystemScope,
                       "The Salvation Army", "ARCWay");

    settings.beginGroup("Address");

    settings.setValue("orgName", ui->le_orgName->text());
    settings.setValue("streetNumber", ui->le_streetNo->text());
    settings.setValue("streetName", ui->le_streetName->text());
    settings.setValue("city", ui->le_city->text());
    settings.setValue("province", ui->le_province->text());
    settings.setValue("zip", ui->le_zip->text());
    settings.setValue("phone", ui->le_phone->text());
    settings.setValue("website", ui->le_website->text());

    settings.endGroup();
}

void MainWindow::isAddressSet()
{
    QSettings settings(QSettings::IniFormat, QSettings::SystemScope,
                       "The Salvation Army", "ARCWay");
    bool emptyString = false;

    settings.beginGroup("Address");

    if (settings.value("orgName").toString().length() == 0) emptyString = true;
    if (settings.value("streetNumber").toString().length() == 0) emptyString = true;
    if (settings.value("streetName").toString().length() == 0) emptyString = true;
    if (settings.value("city").toString().length() == 0) emptyString = true;
    if (settings.value("province").toString().length() == 0) emptyString = true;
    if (settings.value("zip").toString().length() == 0) emptyString = true;
    if (settings.value("phone").toString().length() == 0) emptyString = true;
    if (settings.value("website").toString().length() == 0) emptyString = true;

    qDebug() << "empty address info? " << emptyString;

    if (emptyString) {
        if (doMessageBox("Address information is incomplete.\nPlease set your address information from the admin screen.")){
            ui->stackedWidget->setCurrentIndex(EDITADDRESS);
        } else {
            QString tmpStyleSheet=this->styleSheet();
            this->setStyleSheet("");

            QMessageBox msgBox;
            msgBox.setText("The PDF exports and payment receipts will be missing address information.\n\n"
                           "Please set your address information from the admin page.\n\n"
                           "If you wish to leave one of the address fields empty, type the spacebar while in that field,\n"
                           "that will prevent the warning about missing information.");
            msgBox.exec();

            this->setStyleSheet(tmpStyleSheet);
        }
    }
}

QString MainWindow::getOrgName(){
    QSettings settings(QSettings::IniFormat, QSettings::SystemScope,
                       "The Salvation Army", "ARCWay");
    settings.beginGroup("Address");
    QString tmp = settings.value("orgName").toString();
    return settings.value("orgName").toString().length() == 0 ? "" : tmp;
}

QString MainWindow::getStreetNo(){
    QSettings settings(QSettings::IniFormat, QSettings::SystemScope,
                       "The Salvation Army", "ARCWay");
    settings.beginGroup("Address");
    QString tmp = settings.value("streetNumber").toString();
    return settings.value("streetNumber").toString().length() == 0 ? "" : tmp;
}

QString MainWindow::getStreetName(){
    QSettings settings(QSettings::IniFormat, QSettings::SystemScope,
                       "The Salvation Army", "ARCWay");
    settings.beginGroup("Address");
    QString tmp = settings.value("streetName").toString();
    return settings.value("streetName").toString().length() == 0 ? "" : tmp;
}

QString MainWindow::getCity(){
    QSettings settings(QSettings::IniFormat, QSettings::SystemScope,
                       "The Salvation Army", "ARCWay");
    settings.beginGroup("Address");
    QString tmp = settings.value("city").toString();
    return settings.value("city").toString().length() == 0 ? "" : tmp;
}

QString MainWindow::getProvince(){
    QSettings settings(QSettings::IniFormat, QSettings::SystemScope,
                       "The Salvation Army", "ARCWay");
    settings.beginGroup("Address");
    QString tmp = settings.value("province").toString();
    return settings.value("province").toString().length() == 0 ? "" : tmp;
}

QString MainWindow::getZip(){
    QSettings settings(QSettings::IniFormat, QSettings::SystemScope,
                                                "The Salvation Army", "ARCWay");
    settings.beginGroup("Address");
    QString tmp = settings.value("zip").toString();
    return settings.value("zip").toString().length() == 0 ? "" : tmp;
}

QString MainWindow::getPhone(){
    QSettings settings(QSettings::IniFormat, QSettings::SystemScope,
                       "The Salvation Army", "ARCWay");
    settings.beginGroup("Address");
    QString tmp = settings.value("phone").toString();
    return settings.value("phone").toString().length() == 0 ? "" : tmp;
}

QString MainWindow::getWebsite(){
    QSettings settings(QSettings::IniFormat, QSettings::SystemScope,
                       "The Salvation Army", "ARCWay");
    settings.beginGroup("Address");
    QString tmp = settings.value("website").toString();
    return settings.value("website").toString().length() == 0 ? "" : tmp;
}


void MainWindow::saveReceipt(bool booked, QString amtPaid, bool printPDF) {
    qDebug() << "testing passed params:";
    qDebug() << "booked: " << booked;
    qDebug() << "amtPaid: " << amtPaid;
    qDebug() << "printPDF: " << printPDF;

    bool newReceipt = true;

    //receiptid
    QString timestamp;
    //check if receiptid already exists from previous payments of the same booking
    if (receiptid == "") {
        timestamp = QDate::currentDate().toString("yyyyMMdd") + QTime::currentTime().toString("hhmmss");
        receiptid = timestamp;
        qDebug() << "updated receiptid: " << receiptid;
    } else {
        timestamp = receiptid;
        newReceipt = false;
        qDebug() << "using existing receiptid:" << receiptid;
    }

    qDebug() << "using new receipt: " << newReceipt;
    //date
    QString date = QDate::currentDate().toString("yyyy-MM-dd");
    qDebug() << "set date: " << date;

    //time
    QString time = QTime::currentTime().toString("H:mm AP");
    qDebug() << "set time: " << time;

    //start date
    QString startDate = booked == false ? "" : ui->confirmStart->text();
    qDebug() << "set start: " << startDate;

    //end date
    QString endDate = booked == false ? "" : ui->confirmEnd->text();
    qDebug() << "set end: " << endDate;

    //lenght of stay
    QString stayLen = booked == false ? "" : ui->confirmLength->text();
    qDebug() << "set length: " << stayLen;

    //bed type and number
    QString bedString;
    if (booked) {
        QString s = curBook->room;
        QChar c = *(s.end()-1);
//        qDebug() << "last char" << c;
        if (c == 'M') bedString.append("Mat ");
        else if (c == 'B') bedString.append("Bed ");

        QStringList pieces = curBook->room.split( "-" );
        bedString.append(pieces.value( pieces.length() - 1));
        bedString.chop(1);
    } else {
        bedString == "";
    }
    qDebug() << "set bed: " << bedString;

    //room number
    QString roomNo;
    if (booked) {
        QRegExp rx("[-]");
        QString s = curBook->room;
        QStringList spacecodeList = s.split(rx, QString::SkipEmptyParts);
        roomNo = spacecodeList.at(2);
    } else {
        roomNo = "";
    }
    qDebug() << "set room: " << roomNo;

    //program
    QString program;
    program = booked == true ? curBook->program : "";
    qDebug() << "set program: " << program;

    //program description
    QString programDesc;
    if (booked) {
        QSqlQuery progResult = dbManager->getProgramDesc(curBook->program);
        progResult.next();
//        qDebug() << "desc:";
//        qDebug() << progResult.lastError();
//        qDebug() << progResult.value(0).toString();
        programDesc = progResult.value(0).toString();
    } else {
        programDesc = "";
    }
    qDebug() << "set prog desc: " << programDesc;

    //total cost
    QString totalCost = booked == true ? "$"+ui->confirmCost->text() : "";
    qDebug() << "set cost: " << totalCost;

    //total paid
    QString totalPaid = booked == true ? "$"+ui->confirmTotalPaid->text() : "$"+amtPaid;
    qDebug() << "set paid: " << totalPaid;

    //payment or refund
    QString payOrRef;

    payOrRef = trans == nullptr ? "" : trans->transType;
    qDebug() << "set trans type" << payOrRef;

    //payment method
    qDebug() << transType;

    //total owing
    QString owing = booked == true ? "$" + ui->confirmPaid->text() : "$" + QString::number(curClient->balance, 'f', 2);
    qDebug() << "set bal: " << owing;

    //clientid
    qDebug() << "set clientId" << curClientID.toInt();

    //try to save the receipt 3 times in case of connection lost
    if (newReceipt) {
        for (int i = 0; i < 3; i++) {
            bool saved = dbManager->addReceiptQuery( timestamp,
                                           date,
                                           time,
                                           curClientName,
                                           startDate,
                                           endDate,
                                           stayLen,
                                           bedString,
                                           roomNo,
                                           program,
                                           programDesc,
                                           getStreetNo(),
                                           getStreetName(),
                                           getCity(),
                                           getProvince(),
                                           getZip(),
                                           getOrgName(),
                                           totalCost,
                                           transType,
                                           totalPaid,
                                           payOrRef,
                                           owing,
                                           curClientID.toInt());
            qDebug() << "insert query success: " << saved;
            if (saved) break;
        }
    } else {
        for (int i = 0; i < 3; i++) {
            bool saved = dbManager->updateReceiptQuery( timestamp,
                                           date,
                                           time,
                                           curClientName,
                                           startDate,
                                           endDate,
                                           stayLen,
                                           bedString,
                                           roomNo,
                                           curBook->program,
                                           programDesc,
                                           totalCost,
                                           transType,
                                           totalPaid,
                                           trans->transType,
                                           owing);
            qDebug() << "update query success: " << saved;
            if (saved) break;
        }
    }

    if (printPDF) {
        qDebug() << "retrieving data from db";
        dbManager->getReceiptThread(timestamp);
        on_actionExport_to_PDF_triggered();
        qDebug() << "pdf generated";
    }
}

void MainWindow::on_adminVal_clicked()
{
    ui->stackedWidget->setCurrentIndex(VALIDATEPAGE);
    addHistory(ADMINPAGE);
    doValidate();
}

void MainWindow::on_valUpdate_clicked()
{
    int selected = ui->valTable->selectionModel()->currentIndex().row();
    if(selected == -1){
        return;
    }
    QString clientId = ui->valTable->item(selected, 0)->text();
    double real = ui->valTable->item(selected, 2)->text().toDouble();
    double expected = ui->valTable->item(selected, 3)->text().toDouble();
    Validate * validate = new Validate(this, clientId, real,expected, usernameLoggedIn, QString::number(currentshiftid));
    validate->exec();
}

//search receipt history when receipt tab is clicked
void MainWindow::searchReceipts(QString clientid, QTableWidget* table){
    qDebug()<<"search booking";

    QSqlQuery receiptQuery = dbManager->listReceiptQuery(clientid);
    displayReceipt(receiptQuery, table);

}

//initialize client booking history table in client info tab
void MainWindow::initClReceiptTable(QTableWidget* table){
    table->setRowCount(0);
    table->clearContents();
//    ui->tableWidget_booking->setMinimumHeight(30*6-1);
}


//display booking history in the table view
void MainWindow::displayReceipt(QSqlQuery results, QTableWidget* table){
    initClReceiptTable(table);
    int colCnt = table->columnCount();
//    int dataCnt = results.record().count();
    int row = table->rowCount();
    while(results.next()){
        table->insertRow(row);
        for(int i=0, j=0; i<colCnt; ++j, ++i){
            //date, time, startDate, refund, payTotal, receiptid
            //0      1       2          3      4         5
            //date, time, type, amount, receiptid
            //0      1     2      3        4
            if (i == 2) {
                QString s;
                s += results.value(j+1).toString();
                if (results.value(j).toString() != ""){
                    if (s.length() > 0){
                        s += " and ";
                    }
                    s += "Booking";
                }
                 table->setItem(row, i, new QTableWidgetItem(s));
                j++;

            } else {
                table->setItem(row, i, new QTableWidgetItem(results.value(j).toString()));
            }
        }
        row++;
    }
}


//display selected receipt
void MainWindow::on_btn_displayReceipt_clicked()
{
    int row = ui->tw_receipts->currentRow();
    receiptid = ui->tw_receipts->item(row, 4)->text();

    qDebug() << "retrieving data from db";
    dbManager->getReceiptThread(receiptid);
    on_actionExport_to_PDF_triggered();
    receiptid = "";
}

void MainWindow::on_btn_cf_displayReceipt_clicked()
{
    int row = ui->tw_cl_receipts->currentRow();
    receiptid = ui->tw_cl_receipts->item(row, 4)->text();

    qDebug() << "retrieving data from db";
    dbManager->getReceiptThread(receiptid);
    on_actionExport_to_PDF_triggered();
    receiptid = "";
}

//enable display receipt button when a row is selected
void MainWindow::on_tw_receipts_itemClicked(QTableWidgetItem *item)
{
    Q_UNUSED(item);
    if(ui->tw_receipts->rowCount() > 0){
        ui->btn_displayReceipt->setEnabled(true);
    }
}

void MainWindow::on_tw_cl_receipts_itemClicked(QTableWidgetItem *item)
{
    Q_UNUSED(item);
    if(ui->tw_cl_receipts->rowCount() > 0){
        ui->btn_cf_displayReceipt->setEnabled(true);
    }
}

void MainWindow::getFullName (QString clientId) {
    QString lName, fName, mName;

    QSqlQuery nameQuery = dbManager->getFullName(clientId);
    qDebug() << "getFullName last error: " << nameQuery.lastError();
    nameQuery.next();

    lName = nameQuery.value(0).toString();
    fName = nameQuery.value(1).toString();
    mName = nameQuery.value(2).toString();

    qDebug() << "lname" << lName;
    qDebug() << "fname" << fName;
    qDebug() << "mname" << mName;

    curClientName = "";
    //MAKE FULL NAME
    if(lName!=NULL)
        curClientName = QString(lName.toUpper());
    if(fName!=NULL){
        if(curClientName!="")
            curClientName += QString(", ");
        curClientName += QString(fName.toUpper());
    }
    if(mName!=NULL){
        if(curClientName!="")
            curClientName += QString(" ");
        curClientName += QString(mName.toUpper());
    }

    qDebug() << "new curclientname " << curClientName;
}

void MainWindow::setCurReceipt(QStringList receipt, bool conn) {
    Q_UNUSED(conn);
    curReceipt.clear();
    for (int i = 0; i < receipt.length(); ++i) {
        curReceipt << receipt[i];
        qDebug() << receipt[i];
    }
}

/*==============================================================================
CHANGE PASSWORD
==============================================================================*/
void MainWindow::on_actionChange_Password_triggered()
{
    ChangePassword *changePwDialog = new ChangePassword();

    connect(changePwDialog, SIGNAL(newPw(QString)), this, SLOT(changeUserPw(QString)));
    changePwDialog->show();

}

void MainWindow::changeUserPw(QString newPw){
    if(dbManager->changePassword(userLoggedIn, newPw)){
        qDebug()<<"PWCHANGED";
        statusBar()->showMessage(QString("Password Changed"), 5000);
        return;
    }
    qDebug()<<"PWNOtCHANGED";
    statusBar()->showMessage(QString("Password Change Fail"), 7000);
}
/*==============================================================================
END CHANGE PASSWORD
==============================================================================*/


void MainWindow::on_btn_reg_searchRS_clicked()
{
    QString building, floor, room, start, end, regEx = "^";

    building = ui->cbo_reg_bldg->currentText();
    floor = ui->cbo_reg_floor->currentText();

    //build regex pattern
    regEx += building == "All" ? ".+-" : building + "-";
    regEx += floor == "All" ? ".+-" : floor + "-";

    if (!ui->rdo_reg_room->isChecked()) {
        room = ui->cbo_reg_room->currentText();

        regEx += room == "All" ? ".+-" : room + "-";
    }

    start = ui->cbo_reg_start->currentText();
    end = ui->cbo_reg_end->currentText();

    //handle end range is smaller than start range
    if (start != "All") {
        if (start.toInt() > end.toInt()) {
            regEx = "invalid query";
        }
    }

    //perform pattern matching for the spacecode
    QRegularExpression pattern(regEx);
    qDebug() << "regex: " << regEx;

    for( int i = 0; i < ui->editLookupTable->rowCount(); ++i ) {
        bool match = false;
        QTableWidgetItem *item = ui->editLookupTable->item( i, 1 );
        //match the b-f-[r]-
        match = pattern.match(item->text()).hasMatch();

        //match the start end range
        if (start != "All" && match) {
            int startNo = start.toInt();
            int endNo = end.toInt();
            qDebug() << "set startNo and endNo";
            int itemNo = ui->rdo_reg_room->isChecked() ? ui->editLookupTable->item(i,10)->text().toInt() :
                                                     ui->editLookupTable->item(i,11)->text().toInt();
            qDebug() << "set itemNo";
            if (itemNo < startNo || itemNo > endNo) {
                match = false;
            }
            qDebug() << "checked range of itemNo against startNo and endNo";
        }
        qDebug() << "match at row " << i << ": " << match;

        //hide rows that don't match pattern
        ui->editLookupTable->setRowHidden( i, !match );
    }
}

void MainWindow::on_btn_regCurDay_clicked()
{
    ui->de_regDate->setDate(QDate::currentDate());
    on_btn_regGo_clicked();
}

void MainWindow::on_btn_regGo_clicked()
{
    QSqlQuery results;
    ui->editLookupTable->setSortingEnabled(false);
    results = ui->de_regDate->date() < QDate::currentDate() ? dbManager->populatePastRegistry(ui->de_regDate->date()) :
                                                              dbManager->populateCurrentRegistry();
    populateRegistry(results);
    bool pastDate = ui->de_regDate->date() < QDate::currentDate();
    ui->editDelete->setEnabled(!pastDate);
    ui->editButton->setEnabled(!pastDate);
}

void MainWindow::on_btn_regFutureBookings_clicked()
{
    ui->editLookupTable->setSortingEnabled(false);
    QSqlQuery results = dbManager->populateFutureRegistry();
    populateRegistry(results);
    ui->editDelete->setEnabled(true);
    ui->editButton->setEnabled(true);
}

void MainWindow::populateRegistry(QSqlQuery results) {
    QStringList headers, cols;
    headers << "Client" << "Space #" << "Start Date" << "Checkout Date" <<
               "Program" << "Cost" << "Monthly" << "" << "" << "" << "RoomNo" << "SpaceNo";
    cols << "ClientName" << "SpaceCode" << "StartDate" << "EndDate" <<
            "ProgramCode" << "Cost" << "Monthly" << "BookingId" << "ClientId" << "SpaceId" << "RoomNo" << "SpaceNo";
    populateATable(ui->editLookupTable, headers, cols, results, false);

    addCurrencyToTableWidget(ui->editLookupTable, 5);

    //hide BookingId, ClientId, SpaceId, RoomNo and SpaceNo
    for (int i = 7; i < 12; i++)
        ui->editLookupTable->hideColumn(i);

    MainWindow::resizeTableView(ui->editLookupTable);

    ui->editLookupTable->setSortingEnabled(true);
}

/*==============================================================================
ROOM HISTORY (START)
==============================================================================*/
void MainWindow::on_roomHistory_search_btn_clicked()
{    
    roomHistStruct.buildingNo = ui->building_cbox->currentText() == "All" ? -1 : ui->building_cbox->currentText().toInt();
    roomHistStruct.floorNo = ui->floor_cbox->currentText() == "All" ? -1 : ui->floor_cbox->currentText().toInt();
    roomHistStruct.roomNo = ui->room_cbox->currentText() == "All" ? -1 : ui->room_cbox->currentText().toInt();
    roomHistStruct.spaceNo = ui->space_cbox->currentText() == "All" ? -1 : ui->space_cbox->currentText().toInt();
    roomHistStruct.startRow = 1;
    roomHistStruct.endRow = 100;

    QSqlQuery query;
    if (dbManager->getRoomHistory(&query, roomHistStruct.buildingNo, 
        roomHistStruct.floorNo, roomHistStruct.roomNo, roomHistStruct.spaceNo, 
        roomHistStruct.startRow, roomHistStruct.endRow))
    {
        QStringList header;
        QStringList cols;
        header << "Client" << "Space Code" << "Program" << "Date" << "Start Date"
               << "End Date" << "Action" << "Employee" << "Shift #" << "Time";
        cols << "ClientName" << "SpaceCode" << "ProgramCode" << "Date" << "StartDate"
             << "EndDate" << "Action" << "EmpName" << "ShiftNo" << "Time";
        populateATable(ui->roomHistory_tableWidget, header, cols, query, false);
        resizeTableView(ui->roomHistory_tableWidget);
    }

    roomHistStruct.totalRowCount = dbManager->bookingHistoryRowCount();
    qDebug() << "booking history row count = " << roomHistStruct.totalRowCount;
    if (roomHistStruct.endRow < roomHistStruct.totalRowCount)
    {
        ui->roomHist_loadNext_button->setEnabled(true);
    }
    else
    {
        ui->roomHist_loadNext_button->setEnabled(false);
    }
}

void MainWindow::on_building_cbox_currentTextChanged(const QString &arg1)
{
    qDebug() << "building text changed";
    const bool blocked = ui->floor_cbox->blockSignals(true);
    Q_UNUSED(arg1);

    QSqlQuery results = dbManager->getFloors(ui->building_cbox->currentText());
    ui->floor_cbox->clear();
    qDebug() << "floor combo data cleared";
    populateCombo(ui->floor_cbox, results);
    qDebug() << "floor combo populated";
    on_floor_cbox_currentTextChanged("");
    ui->floor_cbox->blockSignals(blocked);
}

void MainWindow::on_floor_cbox_currentTextChanged(const QString &arg1)
{
    qDebug() << "floor text changed";
    const bool blocked = ui->room_cbox->blockSignals(true);
    Q_UNUSED(arg1);
    QSqlQuery results = dbManager->getRooms(ui->building_cbox->currentText(), ui->floor_cbox->currentText());
    
    ui->room_cbox->clear();
    qDebug() << "room combo data cleared";
    populateCombo(ui->room_cbox, results);
    qDebug() << "room combo populated";
    on_room_cbox_currentTextChanged("");
    
    ui->room_cbox->blockSignals(blocked);
}

void MainWindow::on_room_cbox_currentTextChanged(const QString &arg1)
{
    qDebug() << "room text changed";
    Q_UNUSED(arg1);
    QSqlQuery results = dbManager->getSpaces(ui->building_cbox->currentText(), ui->floor_cbox->currentText(), ui->room_cbox->currentText());
    ui->space_cbox->clear();
    qDebug() << "space cleared";
    populateCombo(ui->space_cbox, results);
    //results.seek(-1);
    //populateCombo(ui->cbo_reg_end, results);
    qDebug() << "space start end populated";
}

void MainWindow::on_roomHist_loadNext_button_clicked()
{
    roomHistStruct.startRow += 100;
    roomHistStruct.endRow += 100;

    QSqlQuery query;
    if (dbManager->getRoomHistory(&query, roomHistStruct.buildingNo, 
        roomHistStruct.floorNo, roomHistStruct.roomNo, roomHistStruct.spaceNo, 
        roomHistStruct.startRow, roomHistStruct.endRow))
    {
        // QStringList header;
        // QStringList cols;
        // header << "Client" << "Space Code" << "Program" << "Date" << "Start Date"
        //        << "End Date" << "Action" << "Employee" << "Shift #" << "Time";
        // cols << "ClientName" << "SpaceCode" << "ProgramCode" << "Date" << "StartDate"
        //      << "EndDate" << "Action" << "EmpName" << "ShiftNo" << "Time";
        //populateATable(ui->roomHistory_tableWidget, header, cols, query, false);
        int row = roomHistStruct.startRow - 1;
        while(query.next())
        {
            ui->roomHistory_tableWidget->insertRow(row);
            for(int i = 0; i < query.record().count(); i++){
                ui->roomHistory_tableWidget->setItem(row, i, new QTableWidgetItem(query.value(i).toString()));
            }
            row++;
        }

        resizeTableView(ui->roomHistory_tableWidget);
    }

    if (roomHistStruct.endRow < roomHistStruct.totalRowCount)
    {
        ui->roomHist_loadNext_button->setEnabled(true);
    }
    else
    {
        ui->roomHist_loadNext_button->setEnabled(false);
    }

}
/*==============================================================================
ROOM HISTORY (END)
==============================================================================*/


void MainWindow::on_cbo_reg_start_currentTextChanged(const QString &arg1)
{
    ui->cbo_reg_end->setEnabled(arg1 != "All");
    ui->lbl_reg_end->setEnabled(arg1 != "All");
}

void MainWindow::on_cbo_reg_end_currentTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    ui->cbo_reg_end->removeItem(ui->cbo_reg_end->findText("All"));
    on_cbo_reg_start_currentTextChanged(ui->cbo_reg_start->currentText());
}

