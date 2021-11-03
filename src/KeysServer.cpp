//
// The KeysServer functionallity.
// Distibution of keys for Clients and the DataServer
//

#include "utils/aux.h"
#include "KeysServer.h"
#include "Point.h"

Logger keysServerLogger(log_debug, "keysServerLogger");//todo change to log_trace

// Validates the prm value, throwing if invalid
// [prm] Corresponds to the number of entry in mValues table
long KeysServer::validatePrm(long prm) {
    //    if (prm < 0 || prm >= 5) throw std::invalid_argument("prm must be in the interval [0, 4]");
    //todo this says which row is chosen from mValues so prm only needs to be in the range of [0, mValue.size()]
    if (prm < 0 || prm >= 6) throw std::invalid_argument("prm must be in the interval [0, 4]");
    return prm;
};

long KeysServer::correctBitSize(long minimum, long oldBitSize) {
    long newBitSize;
    if (oldBitSize <= 0) newBitSize = minimum;
    else if (oldBitSize > 32) newBitSize = 32;
    else newBitSize = oldBitSize;
    return newBitSize;
};

NTL::Vec<long> KeysServer::calculateMvec(const long *vals) {
    NTL::Vec<long> mvec;
    append(mvec, vals[4]);
    if (vals[5] > 1) append(mvec, vals[5]);
    if (vals[6] > 1) append(mvec, vals[6]);
    return mvec;
};

std::vector<long> KeysServer::calculateGens(const long *vals) {
    std::vector<long> gens;
    gens.push_back(vals[7]);
    if (vals[8] > 1) gens.push_back(vals[8]);
    if (vals[9] > 1) gens.push_back(vals[9]);
    return gens;
};

std::vector<long> KeysServer::calculateOrds(const long *vals) {
    std::vector<long> ords;
    ords.push_back(vals[10]);
    if (abs(vals[11]) > 1) ords.push_back(vals[11]);
    if (abs(vals[12]) > 1) ords.push_back(vals[12]);
    return ords;
};

long KeysServer::calculateLevels(bool bootstrap, long bitSize) {
    return bootstrap
           ? 900
           : 30 * (7 + NTL::NumBits(bitSize + 2)); // that should be enough
};

helib::Context &KeysServer::prepareContext(helib::Context &contxt) {
    if (VERBOSE) {
        cout << "input BIT_SIZE=" << bitSize << endl;
        if (nthreads > 1) cout << "  using " << NTL::AvailableThreads() << " threads\n";
        cout << "computing key-independent tables..." << std::flush;
    }
    contxt.buildModChain(L, c, /*willBeBootstrappable=*/bootstrap);
    if (bootstrap) contxt.enableBootStrapping(mvec);

    buildUnpackSlotEncoding(KeysServer::unpackSlotEncoding, contxt.getEA());

    if (VERBOSE) {
        cout << " done.\n";
        contxt.printout();
    }

    return contxt;
}

void KeysServer::prepareSecKey(helib::SecKey &key) const {
    if (VERBOSE) {
        cout << "\ncomputing key-dependent tables..." << std::flush;
    }
    key.GenSecKey();
    addSome1DMatrices(key); // compute key-switching matrices
    addFrbMatrices(key);
    if (bootstrap) key.genRecryptData();
    if (VERBOSE) cout << " done\n";
};

std::vector<helib::zzX> KeysServer::unpackSlotEncoding; //todo move? already defined in class - check what a 2nd def does

//! KeysServer c'tor: default values
// NOTE: The parameters used in this example code are for demonstration only.
// They were chosen to provide the best performance of execution while
// providing the context to demonstrate how to use the HElib's FHE encryption.
// (The parameters may not provide the security level that might be required by real use/application scenarios.)
//KeysServer::KeysServer(long prm, long bitSize, bool bootstrap, long seed, long nthreads)


long KeysServer::decryptCtxt(const helib::Ctxt &ctxt) const {
    if (ctxt.isEmpty()) return -1; //todo should return 0?

    // Create a plaintext for decryption
    helib::Ptxt<helib::BGV> plaintext_result(context);
    // Decrypt the modified ciphertext
    secKey.Decrypt(plaintext_result, ctxt);

    cout << "decryptCtxt: " << plaintext_result << endl;
    cout << "decryptCtxt[0].getp2r(): " << plaintext_result[0].getp2r() << endl;
    //    cout << "decryptCtxt: "<<plaintext_result.totalProduct()<<endl;
    //    cout << "decryptCtxt: "<<plaintext_result.totalSums()<<endl;

    for (int i = 0; i < plaintext_result.size(); ++i) {
        auto x = plaintext_result[i];
        printNameVal(x);
    }

    return long(plaintext_result[0]); //fixme
    //    NTL::ZZX pp;
    //    secKey.Decrypt(pp, ctxt);
    //    return IsOne(pp);
}

//long KeysServer::decryptNum(EncryptedNum cNum, bool isProduct) {
long KeysServer::decryptNum(const EncryptedNum &cNum) const {
    if (!cNum.size()) return -1; //todo should return 0?
    long pNum = 0;
    NTL::ZZX pp;
    //    int out_size = (1 + isProduct) * BIT_SIZE;
    //    for (int bit = 0; bit < out_size; ++bit) {
    for (int bit = 0; bit < cNum.size(); ++bit) {
        secKey.Decrypt(pp, cNum[bit]);
        //        printNameVal(pp);
        if (IsOne(pp)) pNum += std::pow(2, bit);
        //        printNameVal(pNum);
    }
    return pNum;
}

long KeysServer::decryptSize(const std::vector<CBit> &cSize) const {
    long size = 0;
    NTL::ZZX pp;
    for (const Ctxt &ctxt: cSize) {
        secKey.Decrypt(pp, ctxt);
        //        printNameVal(pp);
        size += IsOne(pp);
    }
    return size;
}


const Point KeysServer::scratchPoint() const {
    cout << " scratchPoint" << endl;
    return Point(getPublicKey());//, nullptr);
}

#include <random>

const Point
KeysServer::tinyRandomPoint() const {
    std::random_device rd;
    std::mt19937 mt(rd());
    //    std::uniform_int_distribution<long> dist(0, EPSILON);
    std::uniform_real_distribution<double> dist(0, EPSILON);
    long arr[DIM];
    for (short dim = 0; dim < DIM; ++dim) arr[dim] = dist(mt);
    //    for (short dim = 0; dim < DIM; ++dim) printNameVal(arr[dim]);

    // note that despite the rand illusion, currently this always returns 0
    // which works perfectly for us, but the "real" solution will be ok too
    const Point &point = Point(getPublicKey(), arr);
    return point;
}

const Point
KeysServer::getQuotientPoint(
        const Point &point,
        const std::vector<CBit> &sizeBitVector,
        const short repsNum) const {

    long size = decryptSize(sizeBitVector), arr[DIM];
    printNameVal(size);
    //    if (size)
    long pCoor;
    for (short dim = 0; dim < DIM; ++dim) {
        pCoor = decryptNum(point[dim]);
        arr[dim] = decryptNum(point[dim]) / (repsNum + size);
        printNameVal(pCoor);
        printNameVal(arr[dim]);
    }

    return Point(point.public_key, arr);
}
const EncryptedNum
KeysServer::getQuotient(
        const EncryptedNum &encryptedNum,
        const long num) const {

    long quotient = decryptNum(encryptedNum) / (num);
    EncryptedNum result;
    std::vector<long> quotient_vector = helib::longToBitVector(quotient, BIT_SIZE);
    for (int bit = 0; bit < BIT_SIZE; ++bit)
        getPublicKey().Encrypt(result[bit], NTL::ZZX(quotient_vector[bit]));

    return result;
}



