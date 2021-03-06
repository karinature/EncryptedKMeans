
#ifndef ENCKMEAN_DATASERVER_H
#define ENCKMEAN_DATASERVER_H

#include "Client.h"


using CmpDict =
const std::vector<
        std::unordered_map<
                const Point,
                std::unordered_map<
                        const Point,
                        helib::Ctxt
                >
        >
>;

class DataServer {

protected:
    //    const helib::PubKey &public_key;// = encryptionKey;
    const KeysServer &keysServer;
    const Point tinyRandomPoint;



public:
    /**
     * Constructor for \class{Client},
     * @param keysServer binds to the \class{KeysServer} responsible for the distributing the appropriate key
     * @brief simulates a mini-protocol between the keys server and the Data Server.
     * ks and ds will create a scratch that will allow to encrypt the results:
     * result bit in case of compare, and result num in case of add/multiplication.
     * */
    explicit DataServer(const KeysServer &keysServer) :
            keysServer(keysServer),
            tinyRandomPoint(keysServer.tinyRandomPoint())
    //            ,
    //            retrievedPoints(NUMBER_OF_POINTS)
    //            ,
    //            randomPointsList(DIM) //todo change name to randomPoints
    //            ,
    //            cmpDict(DIM, {{}})
    {
        //        dataServerLogger.log("DataServer()");
        cout << "DataServer()" << endl;
        cmpDict.resize(DIM);
        randomPointsList.resize(DIM);
        retrievedPoints.reserve(NUMBER_OF_POINTS);

    }

    void clearForNextIteration(std::vector<Point> &initial_points) {
        retrievedPoints.clear();
        retrievedPoints.shrink_to_fit();
        for(const Point & p:initial_points) retrievedPoints.push_back(p);

        randomPointsList.clear();
        randomPointsList.resize(DIM);
        for (int dim = 0; dim < DIM; ++dim) {
            randomPointsList[dim].clear();
            randomPointsList[dim].shrink_to_fit();
        }

        //todo - consider not clearing and adding a check in init_dict - if entry exists (from prev iteration) then no need to cmp
//        cmpDict.clear(); // fixme clearing cmpDict creates bug, probably since it's a map and needs to be resized etc.
//        cmpDict.shrink_to_fit();
//        cmpDict.resize(DIM);

        slices.clear();
//        slices.shrink_to_fit();

        slicesMeans.clear();
        slicesMeans.shrink_to_fit();

        farthest.clear();
        farthest.shrink_to_fit();

        minDistanceTuples.clear();
        minDistanceTuples.shrink_to_fit();

        groupsOfClosestPoints.clear();
//        groupsOfClosestPoints.shrink_to_fit();
    }

    /**
     * @brief A simulated retrievel of data from clients.
     * @param clients - a list of clients (chosen by the CA, to share a similar public key).
     * @returns a list of all the points.
    * @return std::vector<Point>
     * * */

    std::vector<Point>
    retrievePoints(
            const std::vector<Client> &clients);

    std::vector<Point> retrievedPoints;
    std::mutex retrievedPointsLock;

    void
    retrievePoints_Thread(const std::vector<Client> &clients);

    std::vector<Point>
    retrievePoints_WithThreads(
            const std::vector<Client> &clients,
            short numOfThreads = NUMBER_OF_THREADS
    );


    std::vector<std::vector<Point> > randomPointsList;

    /**
     * @brief request data from clients and conentrate into one list
     * @param points - all the points from all the clients in the data set
     * @param m - number of random representatives for each slice
     * @returns a list of #DIM lists - each containing m^d randomly chosen points
     * @return std::vector<Point>
     * */
    //    std::vector<std::vector<Point> >
    const std::vector<std::vector<Point> > &
    pickRandomPoints(
            const std::vector<Point> &points,
            int m = 1 / EPSILON
    );

    /**
     * @brief create a comparison dict:
     *   for each 2 points a,b returns the answer to a[dim]>b[dim]
     *   | where a and b are from 2 different groups (one is supgroup of the other),
     *    and dim is the index of the desired coordinate.
     * @param points - a list of all points (in current group).
     * @param randomPoints - a sub group of all points (in current group).
     *  it is a vector of size #DIM, each node is a vector of size m^dim containing random reps
     * @param numOfReps - the desired number of representatives, usually the number of desired data slices.
     * @returns a vector of #DIM dictionaries.
     *   for each #dim the dictionary contains a pair of keys [point1,point2] and the encrypted value [point1[dim]>point[dim].
     * @return std::vector<Point>
     * */
    CmpDict
    createCmpDict(
            const std::vector<Point> &allPoints,
            const std::vector<std::vector<Point> > &randomPoints
    );

    //    CmpDict cmpDict;
    std::vector<
            std::unordered_map<
                    const Point,
                    std::unordered_map<
                            const Point,
                            helib::Ctxt> > > cmpDict;
    std::mutex cmpDictLock;

    void createCmpDict_Dim_Thread(short dim);

    CmpDict &
    createCmpDict_WithThreads(const std::vector<Point> &allPoints,
                              const std::vector<std::vector<Point> > &randomPoints,
                              int numOfThreads = NUMBER_OF_THREADS);

    /**
     * @brief Split into (1/eps) groups - each group is between 2 representative points.
     * @param points - a list of unordered points
     * @param randomPoints - a list of unordered points
     * @param dim - the index of coor by which the comparison is made. in other words - in what dimension the split is made.
     * @returns a list of pairs/tuples of a representative point and a list of the points in its Group (slice/slice).
     * */
    std::map<int, //DIM
            std::vector< //current slices for approp dimension
                    Slice
            >
    >
    splitIntoEpsNet(
            const std::vector<Point> &points,
            const std::vector<std::vector<Point> > &randomPoints,
            const CmpDict &cmpDict
    );

    std::map<int, std::vector<Slice> > slices;
    std::mutex slicesLock;

    void splitIntoEpsNet_R_Thread(
            const Slice &baseSlice,
            const Point &R,
            int dim);

    std::map<int, //DIM
            std::vector<Slice> // slices for approp dimension
    > splitIntoEpsNet_WithThreads(const std::vector<Point> &points,
                                  const std::vector<std::vector<Point> > &randomPoints,
                                  const CmpDict &cmpDict);

    /**
     * @brief calculate cell-means
     * @param slices a list of Cells (each cell is a list of encrypted points and encrypted size)
     * @param keysServer
     * @return a list of slices and their corresponding means
     * @returns std::vector<std::tuple<Point, Slice> >
     * */
//    static
    std::vector<std::tuple<Point, Slice>>
    calculateSlicesMeans(const std::vector<Slice> &slices);

    std::vector<std::tuple<Point, Slice> > slicesMeans;//(slices.size());
    std::mutex slicesMeansLock;

    void calculateSliceMean_Slice_Thread(
            const Slice &slice
    );

    std::vector<std::tuple<Point, Slice> >
    calculateSlicesMeans_WithThreads(
            const std::vector<Slice> &slices
    );

    /**
     * @brief collect mean point from epsNet
     * */
//    static
    std::vector<Point>
    collectMeans(const std::vector<std::tuple<Point, Slice>> &slices);


    //  collect all minimal distances into one place:
    //      for each point
    //          calculate minimal distance from point to all means:
    //              tuple<point, mean, distance> point.mininmal-distance-from(means)
    //              for each mean
    //                  calculate distance from point to mean:
    //                      tuple<point, distance> point.distance-from(mean)
    /**
     * @brief collect all pairs of points and their closest mean point 
     * (not necessarily from the same slice) 
     * by calculating minimal distance from point to one of the means
     * @param points - all original points
     * @param means - all the means from the epsNet
     * @return tuples of [point, closest mean, minimal distance]
     * @returns
     * */
//    static
    std::vector<std::tuple<Point, Point, EncryptedNum>>
    collectMinimalDistancesAndClosestPoints(const std::vector<Point> &points,
                                            const std::vector<Point> &means);

    std::vector<std::tuple<Point, Point, EncryptedNum> > minDistanceTuples;
    std::mutex minDistanceTuplesLock;

    void findMinDist(const Point &point, const std::vector<Point> &means, const KeysServer &keysServer);

    std::vector<std::tuple<Point, Point, EncryptedNum>>
    collectMinimalDistancesAndClosestPoints_WithThreads(
            const std::vector<Point> &points,
            const std::vector<Point> &means
    );

    //  calculate avg distance
    /**
     * @brief calculates the avarage which will be used as a threshold for picking "closest" points
     * @param minDistanceTuples
     * @return Encrypted avrage
     * @returns EncryptedNum
     * */
//    static
    EncryptedNum
    calculateThreshold(
            const std::vector<std::tuple<Point, Point, EncryptedNum>> &minDistanceTuples,
            int iterationNumber);

    //  collect for each mean the points closest to it
    //  for each Point also includes a bit signifying if the point is included returns
//    static
    std::tuple<
            std::unordered_map<long, std::vector<std::pair<Point, CBit> > >,
            std::vector<std::pair<Point, CBit> >,
            std::vector<std::pair<Point, CBit> >
    >
    choosePointsByDistance(
            const std::vector<std::tuple<Point, Point, EncryptedNum> > &minDistanceTuples,
            std::vector<Point> means,
            EncryptedNum &threshold
    );

//    static
    std::tuple<
            std::unordered_map<long, std::vector<std::pair<Point, CBit>>>,
            std::vector<std::pair<Point, CBit>>
    >
    choosePointsByDistance_WithThreads_slower(
            const std::vector<std::tuple<Point, Point, EncryptedNum>> &minDistanceTuples,
            std::vector<Point> &means,
            EncryptedNum &threshold);


    std::unordered_map<
            long, //mean index
            std::vector<std::pair<Point, CBit> > > groupsOfClosestPoints;

    std::vector<std::pair<Point, CBit> > farthest;

    std::mutex groupsLock, farthestLock;

//    static
    std::tuple<
            std::unordered_map<long, std::vector<std::pair<Point, CBit>>>,
            std::vector<std::pair<Point, CBit>>
    >
    choosePointsByDistance_WithThreads(
            const std::vector<std::tuple<Point, Point, EncryptedNum>> &minDistanceTuples,
            std::vector<Point> &means,
            EncryptedNum &threshold);

    void choosePoint_Mean_Thread(Point point, Point meanClosest, std::vector<Point> &means, int i, Ctxt ni);

    void choosePoint_Point_Thread(const std::tuple<Point, Point, EncryptedNum> &tuple, std::vector<Point> &means,
                                  EncryptedNum &threshold);

};


#endif //ENCKMEAN_DATASERVER_H

