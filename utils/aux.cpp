

#include "aux.h"
//#include "properties.h"
#include "src/KeysServer.h"
#include "src/Point.h"
#include "src/Client.h"

#include <sstream>      // std::stringstream
#include <random>

using std::cout;
using std::endl;

std::chrono::time_point<std::chrono::system_clock> NowTime() {
    return CLOCK::now();
}

std::string printDuration(
        const std::chrono::time_point<std::chrono::system_clock> &t1,
        const std::string &funcName) {
    auto t2 = CLOCK::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::string str =
            "\'" + funcName + "\' Finished in " + std::to_string(duration) + " milliseconds.\n";
    cout << str << endl;
    fcout << str << endl;
    return str;
}

std::vector<long> decryptPoint(const Point &p, const KeysServer &keysServer) {
    std::vector<long> pPoint(DIM);
    for (short dim = 0; dim < DIM; ++dim)
        pPoint[dim] = keysServer.decryptNum(p[dim]);
    return pPoint;
}

void printPoint(const Point &p, const KeysServer &keysServer) {
    //    cout << "( ";
    //    for (short dim = 0; dim < DIM; ++dim)
    //        cout << keysServer.decryptNum(p[dim]) << " ";
    ////    cout << ") ";
    //    cout << "id=" << p.id << " cid=" << keysServer.decryptNum(p.cid) << " ) ";
    cout << "(";
    for (short dim = 0; dim < DIM - 1; ++dim)
        cout << keysServer.decryptNum(p[dim]) << ",";
    cout << keysServer.decryptNum(p[DIM - 1]) << "), ";
    //    cout << "id=" << p.id << " cid=" << keysServer.decryptNum(p.cid) << " ) ";
}

void
printPoints(
        const std::vector<Point> &points,
        const KeysServer &keysServer
) {
    cout << "   [ total of " << points.size() << " points ]   ";
    for (const Point &p:points) printPoint(p, keysServer);
}

void printNonEmptyPoints(
        const std::vector<Point> &points,
        const KeysServer &keysServer
) {
    long arr[DIM], cnt = 0;
    for (const Point &p:points) {
        long sum = 0;
        for (short dim = 0; dim < DIM; ++dim) {
            arr[dim] = keysServer.decryptNum(p[dim]);
            sum += arr[dim];
        }
        if (sum) {
            ++cnt;
            printPoint(p, keysServer);
        }
    }
    cout << " \t\t[ total of " << cnt << " points are not empty, out of " << points.size()
         << " ]    ";
}

void
decAndWriteToFile(
        const std::vector<Point> &points,
        const std::string &filename,
        const KeysServer &keysServer
) {
    std::vector<DecryptedPoint> decPoints;
    decPoints.reserve(points.size());
    for (const Point &p : points) decPoints.push_back(decryptPoint(p, keysServer));
    std::ofstream outputFileStream(filename);
    std::stringstream ss;
    long sum;
    for (const DecryptedPoint &p : decPoints) {
        sum = 0;
        ss.str(std::string());
        for (long coor : p) {
            sum += coor;
            double coorD = double(coor) / CONVERSION_FACTOR;
            ss << coorD << " ";
        }
        if (0 < sum) {
            outputFileStream << ss.rdbuf() << endl;
        }
    }
    //    outputFileStream.flush();
    outputFileStream.close();
}

/** @brief Generate random data.
    @returns a vector of clients with random points.
       client [0] stays empty
       client [1] has 1 point - {point0}
       client [2] has 2 points - {point0, point1}
       ...
       client [n] has n points -  {point0, point1, ... , pointN}
*/
std::vector<Client> generateDataClients(const KeysServer &keysServer) {
    //    std::uniform_real_distribution<double> dist(0, NUMBERS_RANGE);
    std::uniform_int_distribution<long> dist(1, NUMBERS_RANGE);
    long tempArr[DIM];
    std::vector<Client> clients(NUMBER_OF_CLIENTS, Client(keysServer));
    for (Client &client:clients) {
        for (int n = 0; n < NUMBER_OF_POINTS / NUMBER_OF_CLIENTS; ++n) {
            for (int dim = 0; dim < DIM; ++dim) tempArr[dim] = randomLongInRange(mt);
            client.encryptPoint(tempArr);
        }
    }
    /*
    //  another option
    for (int i = 1; i < clients.size(); ++i)
        for (int j = 0; j < i; ++j) {
            for (short dim = 0; dim < DIM; ++dim)
                arr[dim] = random() % NUMBERS_RANGE;
            clients[i].encryptPoint(arr);
         }
     */
    return clients;
}

void Slice::printSlice(
        const KeysServer &keysServer)
const {
    cout << "For the Reps: ";
    printPoints(reps, keysServer);
    cout << endl << " These " << keysServer.decryptSize(counter)
         << " Points will be included: ";//<< endl;
    printNonEmptyPoints(points, keysServer);
    //    printPoints(points, keysServer);
    cout << "   ---     --- " << endl;
}

Slice &Slice::addPoint(
        const Point &point,
        const Ctxt &isIncluded) {
    points.push_back(point);
    counter.push_back(isIncluded);
    pointTuples.emplace_back(point, isIncluded);
    return *this;
}

std::vector<Ctxt> prefix(
        const std::vector<Ctxt> &v,
        long k) {
    std::vector<Ctxt> pref;
    pref.reserve(k);
    for (int i = 0; i < k; ++i)
        pref.push_back(v[i]);
    return pref;
}

std::vector<Ctxt> suffix(
        const std::vector<Ctxt> &v,
        long k) {
    std::vector<Ctxt> pref;
    pref.reserve(k);
    for (int i = k; i < v.size(); ++i)
        pref.push_back(v[i]);
    return pref;
}

Ctxt isEqual(
        const std::vector<Ctxt> &a,
        const std::vector<Ctxt> &b,
        long w
) {

    const helib::PubKey &public_key = a.front().getPubKey();
    Ctxt res(public_key);
    Ctxt temp(public_key);
    res.addConstant(1L);    //  res = 1

    for (int i = 0; i < w; ++i) {
        temp = a[i];    //  a[i]
        temp += b[i];   //  a[i]+b[i]
        temp.addConstant(1L);   //  a[i]+b[i]+1
        res *= temp;
    }

    return res;
}
//std::vector<Ctxt> one_plus_b_dot_a(
//        const std::vector<Ctxt> &a,
//        const std::vector<Ctxt> &b,
//        long w
//        ){
//    std::vector<Ctxt> res;
//    res.reserve(w);
//    for (int i = 0; i < ; ++i) {
//
//    }
//}
Ctxt isGrt(const std::vector<Ctxt> &a, const std::vector<Ctxt> &b, long w,
           const helib::SecKey &secret_key) {

    const helib::PubKey &public_key = a.front().getPubKey();
    Ctxt res(public_key);
    Ctxt temp(public_key);

    res.addConstant(1L);    //  res = 1
    res += b[w - 1];            //  res = 1+b[i]
    res *= a[w - 1];            //  res = ( 1+b[i] ) * a[i]
    for (int i = 0; i < w - 1; ++i) {
        temp.addConstant(1L);   //  temp = 1
        temp += b[w - 1];           //  temp = 1+b[i]
        temp *= a[w - 1];           //  temp = ( 1+b[i] ) * a[i]
        temp *= isEqual(suffix(a, i), suffix(b, i), w - 1 - i);

        res += temp;
    }

    return res;
}
