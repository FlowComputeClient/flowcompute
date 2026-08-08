#ifndef PARSER_DECOMPOSE_PAR_DICT_H_
#define PARSER_DECOMPOSE_PAR_DICT_H_

#include <QObject>
#include <QString>

#include "open_foam_dictionary.h"

namespace CaseIO {

struct ParallelConfig {
    Q_GADGET

 public:
    enum class DecompositionMethod {
        Scotch,
        Metis,
        Simple,
        Hierarchical
    };
    Q_ENUM(DecompositionMethod)

    bool useParallel = false;
    unsigned int numSubdomains = 4;
    DecompositionMethod method = DecompositionMethod::Scotch;
    unsigned int nx = 2; unsigned int ny = 2; unsigned int nz = 1;
    QString order = "xyz";
    double delta = 0.001;
};

// Parse decomposeParDict
void parseDecomposeParDict(std::shared_ptr<OpenFoamDictionary> dict,
                            ParallelConfig& config);

// Create new decomposeParDict from a structure
QString createDecomposeParDict(const ParallelConfig& cfg,
                               const QString& openFoamPath);

// Create new decomposeParDict from the number of cores
QString createDecomposeParDict(const QString& openFoamPath, int numCores);
};

#endif  // PARSER_DECOMPOSE_PAR_DICT_H_
