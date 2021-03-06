
#ifndef ENCRYPTEDKMEANS_POINT_H
#define ENCRYPTEDKMEANS_POINT_H

#include <iostream>
#include <helib/helib.h>
#include <helib/binaryCompare.h>
#include <helib/binaryArith.h>
#include <NTL/ZZX.h>

#include "utils/aux.h" // for including KeysServer.h
#include "KeysServer.h"

static Logger loggerPoint(log_debug, "loggerPoint");

static long counter = 0; //fixme move to the cpp file

class Point {
    friend class Client;

    friend class KeysServer;

public:

    //! @var long id
    //! used in createCmpDict for comparison
    const long id;
    Point *originalPointAddress;
    EncryptedNum cid; //    Ctxt cid;
    //    Ctxt &cidref;    //    Ctxt *cidptr;

    //     helib::PubKey &public_key;// = encryptionKey;
    const helib::PubKey &public_key;// = encryptionKey;
    std::vector<EncryptedNum> cCoordinates;

    /*
     Each bit of the binary number is encoded into a single ciphertext. Thus
     for a 16 bit binary number, we will represent this as an array of 16
     unique ciphertexts.
     i.e. b0 = [0] [0] [0] ... [0] [0] [0]        ciphertext for bit 0
          b1 = [1] [1] [1] ... [1] [1] [1]        ciphertext for bit 1
          b2 = [1] [1] [1] ... [1] [1] [1]        ciphertext for bit 2
     These 3 ciphertexts represent the 3-bit binary number 110b = 6
     Note: several numbers can be encoded across the slots of each ciphertext
     which would result in several parallel slot-wise operations.
     For simplicity we place the same data into each slot of each ciphertext,
     printing out only the back of each vector.
     NB: fifteenOrLess4Four max is 15 bits. Later in the code we pop the MSB.
            long BIT_SIZE = 16;
            long OUT_SIZE = 2 * BIT_SIZE;
     */
    explicit Point(const helib::PubKey &public_key, const long coordinates[] = nullptr) :
    //todo maybe better to init to 0, depending future impl & use
            cmpCounter(0), addCounter(0), multCounter(0),
            public_key(public_key),
            id(counter++),
//            cid(std::log2(id)+1, Ctxt(public_key)),
            cid(CID_BIT_SIZE, Ctxt(public_key)),
            pubKeyPtrDBG(&public_key),
            cCoordinates(DIM, std::vector(BIT_SIZE, helib::Ctxt(public_key))) {
        originalPointAddress = this;
        for (long bit = 0; bit < cid.size(); ++bit)
            this->public_key.Encrypt(cid[bit],
                                     NTL::to_ZZX((id >> bit) & 1));
        pCoordinatesDBG.reserve(DIM);
        //        cout << " Point Init" << endl;
        if (coordinates) {
            isEmptyDBG = false;
            for (short dim = 0; dim < DIM; ++dim) {
                pCoordinatesDBG.push_back(coordinates[dim]);
                // Extract the i'th bit of coordinates[dim]
                //  for (long bit = 0; bit < BIT_SIZE; ++bit)
                for (long bit = 0; bit < cCoordinates[dim].size(); ++bit)
                    this->public_key.Encrypt(cCoordinates[dim][bit],
                                             NTL::to_ZZX((coordinates[dim] >> bit) & 1));

            }
        }

        //        todo notice this helibs function:  (CT 24.oct.2021)
        //        /**
        //         * @brief Returns a number as a vector of bits with LSB on the left.
        //         * @param num Number to be converted.
        //         * @param bitSize Number of bits of the input and output.
        //         * @return CBit vector representation of num.
        //         * @note `bitSize` must be non-negative.
        //         **/
        //        std::vector<long> longToBitVector(long num, long bitSize);
    }

    explicit Point(const std::vector<EncryptedNum> &cCoordinates) :
            //todo maybe better to init to 0, depending future impl & use
            cmpCounter(0), addCounter(0), multCounter(0),
            public_key(cCoordinates[0][0].getPubKey()),
            id(counter++),
            cid(CID_BIT_SIZE, Ctxt(cCoordinates[0][0].getPubKey())),
            pubKeyPtrDBG(&public_key),
            pCoordinatesDBG(DIM)
    //            ,
    //            cCoordinates(cCoordinates)
    //  cCoordinates(DIM, std::vector(BIT_SIZE, helib::Ctxt(public_key)))
    {
        originalPointAddress = this;
        for (long bit = 0; bit < cid.size(); ++bit)
            this->public_key.Encrypt(cid[bit],
                                     NTL::to_ZZX((id >> bit) & 1));

        //        this->pCoordinatesDBG.reserve(DIM);
        this->cCoordinates.resize(DIM);//reserve(DIM);
        for (int dim = 0; dim < DIM; ++dim) {
            //            this->cCoordinates.push_back(cCoordinates[dim]);
            vecCopy(this->cCoordinates[dim], cCoordinates[dim]);
            //            this->cCoordinates[dim] = cCoordinates[dim];
        }
        //        cout << "Point c'tor from encrypted data" << endl;

    }

    bool isEmpty() const {
        return cCoordinates[0][0].isEmpty();
    }

    Point(const Point &point) :
    //todo maybe better to init to 0, depending future impl & use
            cmpCounter(point.cmpCounter),
            addCounter(point.addCounter),
            multCounter(point.multCounter),
            public_key(point.public_key),
            //todo make sure this approach won't cuase some unpredictable behaviour later
            // (e.g in cases of EXPLICIT copy (instead of implicit, in which this way makes sense))
            id(point.id),
            cid(point.cid),//.Encrypt(cid,NTL::ZZX(id))),
            //            cidref(point.cidref),
            originalPointAddress(point.originalPointAddress),
            pubKeyPtrDBG(&(point.public_key)),
            cCoordinates(point.cCoordinates),
            pCoordinatesDBG(point.pCoordinatesDBG),
            isEmptyDBG(point.isEmptyDBG),
            isCopyDBG(true) {
        //        cout << " Point copy copy" << endl; //this print is important for later. efficiency...
        if (!point.isEmpty())
            for (short dim = 0; dim < DIM; ++dim) {
                vecCopy(cCoordinates[dim], point.cCoordinates[dim]); //helibs version of vec copy //fixme throws an exception sometimes
//                                cCoordinates[dim] = point.cCoordinates[dim];
            }
        else std::cerr << "point is empty!" << endl;

    }

    Point &operator=(const Point &point) {
        //            cout << " Point assign" << endl;
        if (&point == this || point.isEmpty()) return *this;
        //                id=point.id;
        cid = point.cid;
        originalPointAddress = point.originalPointAddress;
        for (short dim = 0; dim < DIM; ++dim) {
            //   cCoordinates[dim] = point.cCoordinates[dim];
            vecCopy(cCoordinates[dim], point.cCoordinates[dim]); //helibs version of vec copy
            if (!point.pCoordinatesDBG.empty())
                pCoordinatesDBG[dim] = point.pCoordinatesDBG[dim];
        }
        return *this;
    }

    const EncryptedNum &operator[](short int i) const {
        //        if (isEmpty()) return EncryptedNum(helib::Ctxt(public_key));
        return cCoordinates[i];
    }

    EncryptedNum operator[](short int i) {
        //        if (isEmpty()) return EncryptedNum(helib::Ctxt(public_key));
        return cCoordinates[i];
    }

    Point operator+(Point &point) {

        if (point.isEmpty()) {
            std::cerr << "point is empty" << endl;
            return *this; //todo consider
        }
        if (this->isEmpty()) {
            std::cerr << "this is empty" << endl;
            return point;
        }
        Point sum(this->public_key);
        //        cout << "operator+ sum.id is: "<<id<<endl;
        helib::CtPtrs_vectorCt cid_wrapper(sum.cid);
        helib::addTwoNumbers(
                cid_wrapper,
                helib::CtPtrs_vectorCt(point.cid),
                helib::CtPtrs_vectorCt(this->cid),
                CID_BIT_SIZE,   // sizeLimit=0 means use as many bits as needed.
                &(KeysServer::unpackSlotEncoding) // Information needed for bootstrapping.
        );
        for (short dim = 0; dim < DIM; ++dim) {
            helib::CtPtrs_vectorCt result_wrapper(sum.cCoordinates[dim]);
            //             * @brief Adds two numbers in binary representation where each ciphertext of the
            //             * input vector contains a bit.
            //             * @param sum result of the addition operation.
            //             * @param lhs left hand side of the addition.
            //             * @param rhs right hand side of the addition.
            //             * @param sizeLimit number of bits to compute on, taken from the least
            //             * significant end.
            //             * @param unpackSlotEncoding vector of constants for unpacking, as used in
            //             * bootstrapping.
            //             *
            helib::addTwoNumbers(
                    result_wrapper,
                    helib::CtPtrs_vectorCt(point.cCoordinates[dim]),
                    helib::CtPtrs_vectorCt(this->cCoordinates[dim])
                    //                    ,
                    //                    OUT_SIZE,   // sizeLimit=0 means use as many bits as needed.
                    //                    &(KeysServer::unpackSlotEncoding) // Information needed for bootstrapping.
            );
            sum.pCoordinatesDBG[dim] += point.pCoordinatesDBG[dim];
        }
        return sum;
    }

    Point operator+(Point point) {

        if (point.isEmpty()) return *this; //todo consider
        if (this->isEmpty()) return point;
        Point sum(this->public_key);
        //        cout << "operator+ sum.id is: "<<id<<endl;
        helib::CtPtrs_vectorCt cid_wrapper(sum.cid);
        helib::addTwoNumbers(
                cid_wrapper,
                helib::CtPtrs_vectorCt(point.cid),
                helib::CtPtrs_vectorCt(this->cid),
                CID_BIT_SIZE,   // sizeLimit=0 means use as many bits as needed.
                &(KeysServer::unpackSlotEncoding) // Information needed for bootstrapping.
        );
        for (short dim = 0; dim < DIM; ++dim) {
            helib::CtPtrs_vectorCt result_wrapper(sum.cCoordinates[dim]);
            //             * @brief Adds two numbers in binary representation where each ciphertext of the
            //             * input vector contains a bit.
            //             * @param sum result of the addition operation.
            //             * @param lhs left hand side of the addition.
            //             * @param rhs right hand side of the addition.
            //             * @param sizeLimit number of bits to compute on, taken from the least
            //             * significant end.
            //             * @param unpackSlotEncoding vector of constants for unpacking, as used in
            //             * bootstrapping.
            //             *
            helib::addTwoNumbers(
                    result_wrapper,
                    helib::CtPtrs_vectorCt(point.cCoordinates[dim]),
                    helib::CtPtrs_vectorCt(this->cCoordinates[dim]),
                    0,   // sizeLimit=0 means use as many bits as needed.
                    &(KeysServer::unpackSlotEncoding) // Information needed for bootstrapping.
            );
            sum.pCoordinatesDBG[dim] += point.pCoordinatesDBG[dim];
        }
        return sum;
    }

    //// Calculates the sum of many numbers using the 3-for-2 method
    static Point addManyPoints(const std::vector<Point> &points, const KeysServer &keysServer) {
        //        if (points.empty()) return static_cast<Point>(nullptr);
        //        Point sum(points.back().public_key);

        std::vector<std::vector<EncryptedNum> >
                summandsVec(
                DIM,
                std::vector<EncryptedNum>(
                        points.size(),
                        EncryptedNum(
                                BIT_SIZE,
                                helib::Ctxt(
                                        points[0].public_key))));
        std::vector<EncryptedNum> encrypted_results(DIM);

        for (short dim = 0; dim < DIM; ++dim) {
            summandsVec[dim].reserve(points.size());
            for (const Point &point : points) {
                summandsVec[dim].push_back(point.cCoordinates[dim]);
                //                sum.pCoordinatesDBG[dim] += point.pCoordinatesDBG[dim];
            }
            helib::CtPtrMat_vectorCt summands_wrapper(summandsVec[dim]);

            helib::CtPtrs_vectorCt result_wrapper(encrypted_results[dim]);
            /*
             * @brief Sum an arbitrary amount of numbers in binary representation.
             * @param sum result of the summation.
             * @param numbers values of which to sum.
             * @param sizeLimit number of bits to compute on, taken from the least
             * significant end.
             * @param unpackSlotEncoding vector of constants for unpacking, as used in
             * bootstrapping.
             *
             * Calculates the sum of many numbers using the 3-for-2 method.
             **/
            // Calculates the sum of many numbers using the 3-for-2 method
            addManyNumbers(
                    result_wrapper,
                    summands_wrapper
                    //                    ,
                    //                        0,//BIT_SIZE * points.size() * BIT_SIZE, // sizeLimit=0 means use as many bits as needed.
                    //                    &(KeysServer::unpackSlotEncoding) // Information needed for bootstrapping.
            );
            //            sum.cCoordinates[dim] = encrypted_result;
            //            vecCopy(sum.cCoordinates[dim], encrypted_results[dim]);
        }
        Point sum(encrypted_results);

        for (short dim = 0; dim < DIM; ++dim)
            for (const Point &point : points)
                sum.pCoordinatesDBG[dim] += point.pCoordinatesDBG[dim];

        return sum;
    }

    /**
     * @brief Multiplies an encrypted point by an encrypted bit
     * */
    Point operator*(const Ctxt &bit) const {
        //        if (bit.isEmpty()) return *this; //todo consider
        //        if (this->isEmpty()) return Point(this->public_key);
        Point product(*this);

        //        cout << "operator+ sum.id is: "<<id<<endl;
        helib::CtPtrs_vectorCt cid_wrapper(product.cid);
        binaryMask(cid_wrapper, bit);

        for (short dim = 0; dim < DIM; ++dim) {
            helib::CtPtrs_vectorCt result_wrapper(product.cCoordinates[dim]);
            binaryMask(result_wrapper, bit);
            //                for (long i = 0; i < resSize; i++)
            //                    productCoors[i]->multiplyBy(*(lhs[0]));

        }
        return product;
    }

    Point &operator*=(const Ctxt &bit) {
        //        if (bit.isEmpty()) return *this; //todo consider
        //        if (this->isEmpty()) return Point(this->public_key);
        //        cout << "operator+ sum.id is: "<<id<<endl;
        helib::CtPtrs_vectorCt cid_wrapper(
                this->cid); //fixme DANGER ZONE maybe needs to be left w/out change
        binaryMask(cid_wrapper, bit);

        for (short dim = 0; dim < DIM; ++dim) {
            helib::CtPtrs_vectorCt result_wrapper(this->cCoordinates[dim]);
            binaryMask(result_wrapper, bit);
            //                for (long i = 0; i < resSize; i++)
            //                    productCoors[i]->multiplyBy(*(lhs[0]));

        }
        return *this;
    }

    // todo maybe unused. delete?
    Point operator*(Point &point) {
        if (point.isEmpty()) return *this;
        if (this->isEmpty()) return point;
        const long arr[] = {0, 0};
        Point product(this->public_key, arr);
        for (short dim = 0; dim < DIM; ++dim) {
            helib::CtPtrs_vectorCt result_wrapper(product.cCoordinates[dim]);
            helib::multTwoNumbers(
                    result_wrapper,
                    helib::CtPtrs_vectorCt(point.cCoordinates[dim]),
                    helib::CtPtrs_vectorCt(this->cCoordinates[dim]),
                    false,
                    OUT_SIZE,   // sizeLimit=0 means use as many bits as needed.
                    &(KeysServer::unpackSlotEncoding) // Information needed for bootstrapping.
            );
        }
        return product;
    }

    /**
     * @brief compares 2 points by comparing the values of 2 coordinates, in a specified dimention.
     * @param point - a point with encrypted coordinate values.
     * @param currentDim - the index of the coordinates to be compared.
     * @returns a tuple that answers - ((p1[d]>p2[d]), (p2[d]>p1[d])). value are encrypted.
     * @return EncryptedNum
     * */
    std::vector<CBit>
    isBiggerThan(const Point &point, short int currentDim = DIM - 1) const {
        Ctxt mu(public_key), ni(public_key);
        if (!(isEmpty() || point.isEmpty())) {
            if (point.id == id) {
                // note good solution but results in representatives being picked twice
                //  - once for their own group and once for the group above
                public_key.Encrypt(mu, NTL::to_ZZX((true)));
                public_key.Encrypt(ni, NTL::to_ZZX((true))); // make sure
            } else {
                EncryptedNum c = (*this)[currentDim];
                EncryptedNum coor = point[currentDim];
                compareTwoNumbers(mu,
                                  ni,
                                  helib::CtPtrs_vectorCt(c),
                                  helib::CtPtrs_vectorCt(coor)
                                  ,
                                  false,
                                  &KeysServer::unpackSlotEncoding
                );
            }
        }
        return std::vector<CBit>{mu, ni};
        /* todo notice there is also // comparison with max and min
         *  maybe useful later
            and this one too:
                //                    /**
                // * @brief Compute a bitwise NOT of `input`.
                // * @param output Result of bit-flipping `input`.
                // * @param input Binary number to be bit-flipped.
                // * @note The size of `output` and `input` must be the same.
                //
                //        void bitwiseNot(CtPtrs& output, const CtPtrs& input);
                */
    }

    bool operator==(const Point &point) const {
        //        return cid == point.cid;
        return id == point.id;
    }

    /**
     * @brief return the encrypted (square of the) distance
     * @param point from which we measure our distance
     * @return encrypted (square of the) distance from point
     * @return EncryptedNum
     * */
    EncryptedNum
    distanceFrom(
            const Point &point,
            const KeysServer &keysServer
    ) const {
        // this creates a problem when the diff between the original values is negative
        //   (meaning when c1 is smaller then c2) -
        //   the result of `subtractBinary` is a (positive) 2's compliment.
        //   in other words it would be
        //   sub_result = [ NUMBERS_RANGE + (c1-c2) ] mod NUMBERS_RANGE
        // two options to fix:
        //  1. use helib cmp w/ min/max ptrs <<---------------- this version is much faster (X4)
        //  2. calculate (c1^2)+(c2^2)-2(c1*c2) = (c1-c2)^2
        //  3. consult adidanny

        // c1 = p1.coor[dim]      
        // c2 = p2.coor[dim]

        /**     option 1    */
        auto t0_distanceFrom_cmp_version = CLOCK::now();

        std::vector<EncryptedNum> sqaredDiffs(DIM);
        for (int dim = 0; dim < DIM; ++dim) {

            EncryptedNum thisCoor = this->cCoordinates[dim];
            helib::CtPtrs_vectorCt p1c(thisCoor);
            EncryptedNum pointCoor = point.cCoordinates[dim];
            helib::CtPtrs_vectorCt p2c(pointCoor);
            EncryptedNum eMax, eMin;//(BIT_SIZE, helib::Ctxt(public_key));
            helib::CtPtrs_vectorCt max(eMax), min(eMin);

            // max{(c1-c2),(c2-c1)} will be equal to |c1 - c2|
            helib::Ctxt mu(public_key), ni(public_key);
            helib::compareTwoNumbers(max, min,
                                     mu, ni,
                                     p1c,
                                     p2c
                    //                                     ,false
                    //                                     , &(KeysServer::unpackSlotEncoding)
            );
            // if c1 > c2 use (c1 - c2), else (c2 - c1)

            // subtract: |c1 - c2|
            EncryptedNum sub_vector(BIT_SIZE, helib::Ctxt(public_key));
            helib::CtPtrs_vectorCt sub_wrapper(sub_vector);
            helib::subtractBinary(sub_wrapper,
                                  max,
                                  min);

            // square: |c1 - c2|^2
            helib::CtPtrs_vectorCt sqr_wrapper(sqaredDiffs[dim]);
            helib::multTwoNumbers(sqr_wrapper,
                                  sub_wrapper,
                                  sub_wrapper
                    //                                  , true
            );

            //            printNameVal(keysServer.decryptNum(p1c.v));
            //            printNameVal(keysServer.decryptNum(p2c.v));
            //            printNameVal(keysServer.decryptNum(eMax));
            //            printNameVal(keysServer.decryptNum(eMin));
            //            printNameVal(keysServer.decryptNum(sub_wrapper.v));
            //            printNameVal(keysServer.decryptNum(sqaredDiffs[dim]));

        }

        // sum: SUM[ ( c1-c2 )^2 | for all dim ]
        helib::CtPtrMat_vectorCt summands_wrapper(sqaredDiffs);
        EncryptedNum result_vector;//(BIT_SIZE, helib::Ctxt(public_key));
        helib::CtPtrs_vectorCt output_wrapper(result_vector);
        helib::addManyNumbers(output_wrapper, summands_wrapper);
        //        printNameVal(keysServer.decryptNum(result_vector));

        loggerPoint.log(
                printDuration(t0_distanceFrom_cmp_version, "distanceFrom (cmp version)"));

        /**     option 2    */
        /*
                auto t0_long_version = CLOCK::now();

                std::vector<EncryptedNum >
                        parts(DIM,
                              EncryptedNum(2 * BIT_SIZE,
                                                       helib::Ctxt(point.public_key)));

                for (int dim = 0; dim < DIM; ++dim) {
                    // subtract: c1 - c2
                    helib::CtPtrs_vectorCt p1c(this->cCoordinates[dim]);
                    helib::CtPtrs_vectorCt p2c(point.cCoordinates[dim]);

                    //  (c1) ^ 2
                    EncryptedNum c1sqr;
                    helib::CtPtrs_vectorCt sqr_wrapper1(c1sqr);
                    helib::multTwoNumbers(sqr_wrapper1,
                                          p1c,
                                          p1c,
                                          false,
                                          2 * BIT_SIZE
                    );
                    //            printNameVal(keysServer.decryptNum(c1sqr));

                    //  (c2) ^ 2
                    EncryptedNum c2sqr;
                    helib::CtPtrs_vectorCt sqr_wrapper2(c2sqr);
                    helib::multTwoNumbers(sqr_wrapper2,
                                          p2c,
                                          p2c,
                                          false,
                                          2 * BIT_SIZE
                    );
                    //            printNameVal(keysServer.decryptNum(c2sqr));

                    //  (c1)^2 + (c2)^2
                    EncryptedNum sumSqrs;
                    helib::CtPtrs_vectorCt sum_wrapper(sumSqrs);
                    helib::addTwoNumbers(sum_wrapper,
                                         sqr_wrapper1,
                                         sqr_wrapper2,
                                         2 * BIT_SIZE
                    );
                    //            printNameVal(keysServer.decryptNum(sumSqrs));

                    //  (c1) * (c2)
                    EncryptedNum multC1C2;
                    helib::CtPtrs_vectorCt mult_wrapper(multC1C2);
                    helib::multTwoNumbers(mult_wrapper,
                                          p1c,
                                          p2c,
                                          false,
                                          2 * BIT_SIZE
                    );
                    //            printNameVal(keysServer.decryptNum(multC1C2));

                    //  2 * [(c1) * (c2)]
                    EncryptedNum dblMultC1C2;
                    helib::CtPtrs_vectorCt dbl_wrapper(dblMultC1C2);
                    helib::addTwoNumbers(dbl_wrapper,
                                         mult_wrapper,
                                         mult_wrapper,
                                         2 * BIT_SIZE
                    );
                    //            printNameVal(keysServer.decryptNum(dblMultC1C2));

                    // sum: (c1^2) + (c2^2) - 2(c2*c2)
                    helib::CtPtrs_vectorCt output(parts[dim]);
                    helib::subtractBinary(output,
                                          sum_wrapper,
                                          dbl_wrapper);
                    //            printNameVal(keysServer.decryptNum(parts[dim]));
                }

                // sum: SUM[ ( c1-c2 )^2 | for all dim ]
                helib::CtPtrMat_vectorCt summands_wrapper2(parts);
                EncryptedNum result_vector2;
                helib::CtPtrs_vectorCt output_wrapper2(result_vector2);
                helib::addManyNumbers(output_wrapper2,
                                      summands_wrapper2);
                //        printNameVal(keysServer.decryptNum(result_vector));

                loggerPoint.log(printDuration(t0_long_version, "dist long version (without compare)"));
                */

        return result_vector;
    }

    /**
     * @brief find closest point, from a list, and minimal distance from it
     * @param points list of points from which we measure our distance
     * @return minimal distance and corresponding closest point
     * @return std::pair<Point, EncryptedNum>
     * */
    std::pair<Point, EncryptedNum>
    findMinDistFromMeans(
            const std::vector<Point> &points,
            const KeysServer &keysServer
    ) const {
        auto t0_minDist = CLOCK::now();

        //  Collect Distancses from Means
        std::vector<std::pair<const Point &, EncryptedNum> > distances;
        distances.reserve(points.size());
        for (const Point &mean:points)
            distances.emplace_back(mean, distanceFrom(mean, keysServer));
        //   distances.push_back(
        //                    std::pair<const Point &, EncryptedNum>(mean, distanceFrom(mean, keysServer)));

        // init minimal distance
        Point closestPoint(distances[0].first);
        EncryptedNum minimalDistance(distances[0].second);

        for (std::pair<const Point &, EncryptedNum> &tuplePointDistance:distances) {

            EncryptedNum eMax,//(BIT_SIZE, helib::Ctxt(public_key)),
            eMin;//(BIT_SIZE, helib::Ctxt(public_key));
            helib::CtPtrs_vectorCt max(eMax), min(eMin);
            helib::CtPtrs_vectorCt distMin(minimalDistance);
            helib::CtPtrs_vectorCt dist(tuplePointDistance.second);

            helib::Ctxt mu(public_key), ni(public_key);

            helib::compareTwoNumbers(max, min,
                                     mu, ni,
                                     distMin,
                                     dist,
                                     false,
                                     &(KeysServer::unpackSlotEncoding));

            //  eMin (and min.v) created by vecCopy
            //  and therefore will always have a different address from tuplePointDistance.first
            //  ... so need to use helib's compare again?
            //            helib::binaryCond(minimalDistance.first, mu, distMin, dist);
            Ctxt negated_cond(mu);
            negated_cond.addConstant(NTL::ZZX(1L));
            //            cout << endl << "min" << endl;
            //            printPoint(minimalDistance.first, keysServer);
            //            cout << endl << "min*(!mu)" << endl;
            //            printPoint(minimalDistance.first * negated_cond, keysServer);
            //            cout << endl << "dist" << endl;
            //            printPoint(tuplePointDistance.first, keysServer);
            //            cout << endl << "dist*(mu)" << endl;
            //            printPoint(tuplePointDistance.first * mu, keysServer);
            //            cout << endl << "min*(!mu) + dist*(mu)" << endl;
            closestPoint = closestPoint * negated_cond + tuplePointDistance.first * mu;
            minimalDistance = eMin;
            //            minimalDistance.first = minimalDistance.first * negated_cond + tuplePointDistance.first * mu;
            //            minimalDistance.second = eMin;
            //        printPoint(closestPoint, keysServer);
            //        printNameVal(keysServer.decryptNum(minimalDistance));
            //            cout << "------------------------" << endl;
            loggerPoint.log(printDuration(t0_minDist,
                                          "tuplePointDistance loop in findMinDistFromMeans"));
        }

        //        cout << "     Final: " << endl;
        //        printPoint(closestPoint, keysServer);
        //        printNameVal(keysServer.decryptNum(minimalDistance));

        loggerPoint.log(printDuration(t0_minDist, "findMinDistFromMeans"));

        return {closestPoint, minimalDistance};
    }


    /*  for DBG */
    std::vector<long> pCoordinatesDBG;

    bool isCopyDBG = false;
    bool isEmptyDBG = true;
    const helib::PubKey *pubKeyPtrDBG;
    long cmpCounter, addCounter, multCounter; //todo
};



/** hash functionality for std::unordered_map of Point  */
namespace std {

    template<>
    struct hash<const Point> {
        std::size_t operator()(const Point &point) const {
            using std::size_t;
            using std::hash;

            return hash<long>()(point.id);
            //            return point.id;
        }
    };

}

struct cmpPoints {
    long id;

    bool operator()(const Point &a, const Point &b) const {
        return a.id > b.id;
    }

    bool operator==(const Point &p) const {
        //        return this == p;
        return id == p.id;
    }
};

struct hashPoints {
    std::size_t operator()(const Point &point) const {
        return std::hash<long>()(point.id);
    }
    //    bool operator==(const Point &p) const {
    //        return id == p.id;
    //    }
};


#endif //ENCRYPTEDKMEANS_POINT_H
