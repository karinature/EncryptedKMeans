//
// Created by karina on 25/07/2021.
//

#include "TestKeysServer.h"
#include "TestPoint.h"
#include "TestClient.h"
#include "TestDataServer.h"

#include "utils/aux.h"

int main(){

    cout << " ============ Test KeysServer ============ " << endl ;
    TestKeysServer::testConstructor();

    cout << " ============ Test Point ============ " << endl ;
    TestPoint::testConstructor();
    TestPoint::testEncryptCoordinates();
    TestPoint::testAddition();
    TestPoint::testMultiplication();

    cout << " ============ Test Client ============ " << endl ;
    TestClient::testConstructor();
    TestClient::testEncryptCoordinates();
    TestClient::testDecryptCoordinates();
    TestClient::testEncryptScratchPoint();

    cout << " ============ Test DataServer ============ " << endl ;
    TestDataServer::testConstructor();
    TestDataServer::testScratchPoint();
    TestDataServer::testCompareClients();

}