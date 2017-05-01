#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QString>
#include <QtSql/QtSql>
#include <stdio.h>
#include <cstdarg>
#include <iostream>
#include <fstream>
#include "shared.h"
//#include "dbconfig.h"
#include <QDesktopServices>
#include <QStringList>
#include <QMutex>

typedef QList<int> IntList;

class DatabaseManager : public QObject
{
    Q_OBJECT
public:

    static QString staticError;
    /*==========================================================================
    DATABASE MANAGER SETUP
    ==========================================================================*/
    explicit DatabaseManager(QObject *parent = 0);
    bool createDatabase(QSqlDatabase* tempDbPtr, QString connName);

    /*==========================================================================
    GENERAL QUERYS
    ==========================================================================*/
    QSqlQuery selectAll(QString tableName);
    void printAll(QSqlQuery queryResults);
    QSqlQuery execQuery(QString queryString);
    bool execQuery(QSqlQuery* query, QString queryString);
    void checkDatabaseConnection(QSqlDatabase* database);
    bool checkDatabaseConnection();

    /*==========================================================================
    FILE DOWNLOAD AND UPLOAD RELATED FUNCTIONS
    ==========================================================================*/
    bool uploadCaseFile(QSqlDatabase* tempDbPtr, QString connName, QString filepath);
    bool downloadLatestCaseFile(QSqlDatabase* tempDbPtr, QString connName);
    QSqlQuery getLatestFileUploadEntry(QSqlDatabase* tempDbPtr, QString tableName);
    int getDbCounter();
    void downloadThread();
    void uploadThread(QString strFilePath);
    void printDbConnections();



    /*==========================================================================
    PROFILE PICTURE UPLOAD AND DOWNLOAD RELATED FUNCTIONS
    ==========================================================================*/
    bool uploadProfilePic(QSqlDatabase* tempDbPtr, QString connName, QImage profilePic);
    void uploadProfilePicThread(QString strFilePath);
    bool downloadProfilePic(QImage* img);
    bool downloadProfilePic2(QImage* img,QString idNum);
    void testuploadProfilePicThread(QString strFilePath);
    bool insertClientWithPic(QStringList* registerFieldList, QImage* profilePic);
    bool insertClientLog(QStringList* registerFieldList);
    bool updateClientWithPic(QStringList* registerFieldList, QString clientId, QImage* profilePic);
    bool deleteClientFromTable(QString tableName, QString ClientId);
    QSqlQuery getCaseWorkerList();
//    QSqlQuery checkUniqueClient(QStringList* infoList);
    QSqlQuery checkUniqueClient(QString fname, QString mname, QString lname, QString dob);
    QSqlQuery checkUniqueSIN(QString sin);


    QSqlQuery searchClientList(QString ClientName);
    QSqlQuery searchTableClientInfo(QString tableName, QString ClientId);
    QSqlQuery searchClientInfo(QString ClientId);
    bool searchClientInfoPic(QImage * img, QString ClientId);
    QSqlQuery searchClientTransList(int maxNum, QString ClientId, int type);
    QSqlQuery searchBookList(int maxNum, QString clientId);
    QSqlQuery searchBookList(int maxNum, QString clientId, int type);
    int countInformationPerClient(QString tableName, QString ClientId);

    /*==========================================================================
    REPORT QUERYS
    ==========================================================================*/
    bool getDailyReportCheckoutQuery(QSqlQuery* queryResults, QDate date);
    bool getDailyReportVacancyQuery(QSqlQuery* queryResults, QDate date);
    bool getDailyReportLunchQuery(QSqlQuery* queryResults, QDate date);
    bool getDailyReportWakeupQuery(QSqlQuery* queryResults, QDate date);
    int getDailyReportEspCheckouts(QDate date);
    int getDailyReportTotalCheckouts(QDate date);
    int getDailyReportEspVacancies(QDate date);
    int getDailyReportTotalVacancies(QDate date);
    void getDailyReportStatsThread(QDate date);
    int getIntFromQuery(QString queryString);
    QStringList getSpaceInfoFromId(int spaceId);
    bool getShiftReportBookingQuery(QSqlQuery* queryResults, QDate date, int shiftNo);
    bool getShiftReportTransactionQuery(QSqlQuery* queryResults, QDate date, int shiftNo);
    bool getShiftReportRefundQuery(QSqlQuery* queryResults, QDate date, int shiftNo);
    bool getShiftReportTotal(QDate date, int shiftNo, QString payType, double* result);
    void getShiftReportStatsThread(QDate date, int shiftNo);
    bool getShiftReportClientLogQuery(QSqlQuery* queryResults, QDate date, int shiftNo);
    bool getShiftReportOtherQuery(QSqlQuery* queryResults, QDate date, int shiftNo);
    bool insertOtherLog(QString empName, int shiftNo, QString logText);
    bool insertCashFloat(QDate date, int shiftNo, QString empName,
        QString comments, QList<int> coins);
    bool getCashFloatReportQuery(QSqlQuery* queryResults, QDate Date, int shiftNo);
    void getCashFloatThread(QDate date, int shiftNo);
    bool getMonthlyReportQuery(QSqlQuery* queryResults, int month, int year);
    void getMonthlyReportThread(int month, int year);
    int getMonthlyUniqueClients(int month, int year);
    bool getYellowRestrictionQuery(QSqlQuery* queryResults);
    bool getRedRestrictionQuery(QSqlQuery* queryResults);
    bool getDoubleFromQuery(QString queryString, double* result);
    void reconnectToDatabase();
    void reconnectToDatabase(QSqlDatabase* database);
    bool getRoomHistory(QSqlQuery* queryResults, int buildingNo, int floorNo,
        int roomNo, int spaceNo, int startRow, int endRow);
    int bookingHistoryRowCount();

    //COLIN STUFF/////////////////////////////////////////////////////////////
    QSqlQuery getCurrentBooking(QDate start, QDate end, QString program);
    QSqlQuery getPrograms();
    bool insertBookingTable(QString insert);
    int getMonthlyRate(QString room, QString program);
    bool addPayment(QString values);
    QSqlQuery getActiveBooking(QString user, bool userLook);
    double getRoomCost(QString roomNo);
    bool updateBooking(QString q);
    QSqlQuery pullClient(QString id);
    bool updateBalance(double d, QString id);
    bool removeTransaction(QString id);
    bool setPaid(QString id, QString chequeNo);
    QSqlQuery getOutstanding();
    QSqlQuery getOwingClients();
    QSqlQuery setLunches(QDate date, int num, QString id, QString room);
    QSqlQuery getLunches(QDate start, QDate end, QString id);
    bool updateLunches(QDate date, int num, QString id);
    bool removeLunches(QDate date, QString id);
    QSqlQuery getWakeups(QDate start, QDate end, QString id);
    bool removeLunchesMulti(QDate date, QString id);
    bool deleteWakeupsMulti(QDate date, QString id);
    bool setWakeup(QDate date, QString time, QString id, QString room);
    bool updateWakeups(QDate date, QString time, QString id);
    bool deleteWakeups(QDate date, QString id);
    QSqlQuery getNextBooking(QDate endDate, QString roomId);
    QSqlQuery getSwapBookings(QDate start, QDate end, QString program, QString maxSwap, QString curBook);
    bool insertIntoBookingHistory(QString clientName, QString spaceId, QString program, QString start, QString end, QString action, QString emp, QString shift, QString clientId);
    bool addHistoryFromId(QString bookId, QString empId, QString shift, QString action);
    QSqlQuery getRoomCosts(QString roomId);
    QSqlQuery getBalance(QString clientId);
    bool updateLunchRoom(QDate startDate, QDate endDate, QString clientId, QString rooomId);
    bool updateWakeupRoom(QDate startDate, QDate endDate, QString clientId, QString rooomId);
    QSqlQuery getBooking(QString bId);
    bool escapePayment(QString clientId, QString curDate, QString amount, QString type, QString notes, QString chequeNo, QString msd, QString issued,
                                        QString transtype, QString outstanding, QString empId, QString shiftNo, QString time);
    QSqlQuery loadStorage(QString id);
    bool addBooking(QString stringStart, QString stringEnd, QString roomId, QString clientId, QString fullName, double cost, QString program);
    QSqlQuery getAllClients();
    double validateMoney(QString clientId);
    QSqlQuery getRoomBooking(QString roomId);
    QSqlQuery getClientBooking(QString clientId);
    QSqlQuery getClientTransactions(QString clientId);
    bool addStorage(QString id, QString name, QString data);
    bool updateStore(QString storeId, QString data);
    bool updateStoreDate(QString storeId);
    QSqlQuery getFullStorage();
    bool removeStorage(QString storeId);
    bool deleteBooking(QString id, QString currEmpName, int currShift);
    QSqlQuery getRole(QString empName);
    double getBookingCost(QString bookId);
    double getDoubleBalance(QString clientId);
    bool checkDoubleBook(QString clientId);
    bool isBanned(QString clientId);
    //END COLIN STUFF///////////////////////////////////////////////////////
    void print();
    QSqlQuery loginSelect(QString username, QString password);
    QSqlQuery findUser(QString username);
    QSqlQuery addNewEmployee(QString username, QString password, QString role, QString name);
    QSqlQuery updateEmployee(QString username, QString password, QString role, QString name);
    QSqlQuery deleteEmployee(QString username, QString password, QString role);
    bool downloadLatestCaseFile();
    QSqlQuery getTransactions(QDate start, QDate end);
    QSqlQuery AddProgram(QString pcode, QString pdesc);
    QSqlQuery updateProgram(QString pcode, QString pdesc);
    QSqlQuery getAvailableBeds(QString pcode);
    QSqlQuery getAssignedBeds(QString pcode);
    QSqlQuery searchSingleBed(QString buildingno, QString floorno, QString roomno, QString spaceno);
    QSqlQuery searchIDInformation(QString buildingno, QString floorno, QString roomno);
    QSqlQuery updateAllSpacecodes();
    QSqlQuery deleteSpace(QString buildingno, QString floorno, QString roomno, QString spaceno);
    QSqlQuery updateSpaceProgram(QString spaceid, QString program);
    QSqlQuery addPcp(int rowid, QString clientId, QString type, QString goal, QString strategy, QString date);
    QSqlQuery addNote(QString clientId, QString notes);
    QSqlQuery updateNote(QString clientId, QString notes);
    QSqlQuery readNote(QString clientId);
    QSqlQuery getProgramDesc(QString programcode);
    void readPcpThread(QString clientId, QString type, int idx);
    bool getPcpQuery(QSqlQuery* query, QString curClientID, QString type);
    bool addReceiptQuery(QString receiptid, QString date, QString time, QString clientName, QString startDate,
                                          QString endDate, QString numNights, QString bedType, QString roomNo, QString prog,
                                          QString descr, QString streetNo, QString streetName, QString city, QString province,
                                          QString zip, QString org, QString totalCost, QString payType, QString payTotal,
                                          QString refund, QString payOwe, int clientId);
    bool updateReceiptQuery(QString receiptid, QString date, QString time, QString clientName, QString startDate,
                                          QString endDate, QString numNights, QString bedType, QString roomNo, QString prog,
                                          QString descr, QString totalCost, QString payType, QString payTotal,
                                          QString refund, QString payOwe);
   bool getReceiptQuery(QSqlQuery* query, QString receiptid);
   void getReceiptThread(QString receiptid);
   QSqlQuery listReceiptQuery(QString clientid);
   QSqlQuery getFullName(QString clientId);

    //Shift table
    bool updateShift(bool shiftExist, QString selectedDay, QStringList *shiftList);
    QSqlQuery getShiftInfoDaily(QString day = "");

    //change user PW
    bool changePassword(QString userName, QString newPassword);

    QSqlQuery getBuildings();
    QSqlQuery getFloors(QString building);
    QSqlQuery getRooms(QString building, QString floor);
    QSqlQuery getSpaces(QString building, QString floor, QString room);
    QSqlQuery populatePastRegistry(QDate date);
    QSqlQuery populateFutureRegistry();
    QSqlQuery populateCurrentRegistry();
    QSqlQuery getCaseFilePath(QString clientId);
    bool setCaseFilePath(QString clientId, QString path);

signals:
    void dailyReportStatsChanged(QList<int> list, bool conn);
    void shiftReportStatsChanged(QStringList list, bool conn);
    void cashFloatChanged(QDate date, int shiftNo, QStringList list, bool conn);
    void cashFloatInserted(QString empName, QString currentDateStr,
        QString currentTimeStr);
    void monthlyReportChanged(QStringList list, bool conn);
    void noDatabaseConnection(QSqlDatabase* database);
    void noDatabaseConnection();
    void reconnectedToDatabase();
    void getPcpData(QStringList goal, QStringList strategy, QStringList date, int idx,  bool conn);
    void getReceiptData(QStringList receipt, bool conn);

private:
    QSqlDatabase db = QSqlDatabase::database();
    static QMutex mutex;
    static int dbCounter;
};

extern DatabaseManager* dbManager;

#endif // DATABASEMANAGER_H
