#include "decompose_par_dict.h"

#include "common.h"

void CaseIO::parseDecomposeParDict(std::shared_ptr<OpenFoamDictionary> dict,
                                 ParallelConfig& config) {
    if (!dict || dict->hasSyntaxErrors()) {
        config.useParallel = false;
        return;
    }
    config.useParallel = true;

    // Parse numberOfSubdomains
    double numSubs = dict->getNumber("numberOfSubdomains");
    if (!std::isnan(numSubs) && numSubs > 0) {
        config.numSubdomains = static_cast<int>(numSubs);
    }

    // Parse decomposition method
    QString methodStr = dict->getString("method").toLower();
    QString coeffsDictName;

    if (methodStr == "scotch") {
        config.method = ParallelConfig::DecompositionMethod::Scotch;
    } else if (methodStr == "metis") {
        config.method = ParallelConfig::DecompositionMethod::Metis;
    } else if (methodStr == "simple") {
        config.method = ParallelConfig::DecompositionMethod::Simple;
        coeffsDictName = "simpleCoeffs";
    } else if (methodStr == "hierarchical") {
        config.method = ParallelConfig::DecompositionMethod::Hierarchical;
        coeffsDictName = "hierarchicalCoeffs";
    }

    // Parse method-specific coefficients
    if (!coeffsDictName.isEmpty()) {
        // Extract the subdivision vector 'n'
        QStringList nList = dict->getList(coeffsDictName + "/n");
        if (nList.size() >= 3) {
            bool okX, okY, okZ;
            int nx = nList[0].toInt(&okX);
            int ny = nList[1].toInt(&okY);
            int nz = nList[2].toInt(&okZ);

            if (okX && okY && okZ) {
                config.nx = nx;
                config.ny = ny;
                config.nz = nz;
            }
        }

        // Extract the cell skew factor 'delta'
        double delta = dict->getNumber(coeffsDictName + "/delta");
        if (!std::isnan(delta)) {
            config.delta = delta;
        }

        // Extract 'order' (Standard for hierarchical)
        if (config.method == ParallelConfig::DecompositionMethod::Hierarchical) {
            QString orderStr = dict->getString(coeffsDictName + "/order");
            if (!orderStr.isEmpty()) {
                config.order = orderStr;
            }
        }
    }
}

QString CaseIO::createDecomposeParDict(const ParallelConfig& cfg,
                                   const QString& openFoamPath) {
    QString dictStr;
    QTextStream out(&dictStr);

    // Write the standard OpenFOAM header
    out << createFoamHeader("decomposeParDict", openFoamPath);

    // Standard lambda with indentation support
    auto writeEntry = [&out](const QString& keyword, const QString& value,
                             int indentLevel = 0, bool addEmptyLine = false) {
        QString indent(indentLevel * 4, ' ');
        out << indent << keyword.leftJustified(20, ' ') << value << ";\n";
        if (addEmptyLine)
            out << "\n";
    };

    // Core decomposition parameters
    writeEntry("numberOfSubdomains",
               QString::number(cfg.numSubdomains), 0, true);

    // OpenFOAM expects lowercase method names (e.g., "scotch", "simple")
    QString methodStr = enumToString(cfg.method, "scotch").toLower();
    writeEntry("method", methodStr, 0, true);

    // Method-specific coefficients block
    QString coeffsDictName = methodStr + "Coeffs";
    out << coeffsDictName << "\n{\n";

    // 'simple' and 'hierarchical' require the 'n' vector and 'delta'
    if (methodStr == "simple" || methodStr == "hierarchical") {
        QString nStr =
            QString("(%1 %2 %3)").arg(cfg.nx).arg(cfg.ny).arg(cfg.nz);
        writeEntry("n", nStr, 1);

        // 'hierarchical' additionally requires the axis ordering
        if (methodStr == "hierarchical") {
            writeEntry("order", cfg.order, 1);
        }

        writeEntry("delta", QString::number(cfg.delta), 1);
    }

    out << "}\n\n";

    // Write closing separator
    out << createFoamFooter();

    return dictStr;
}

// Create a new decomposeParDict file from the number of cores
QString CaseIO::createDecomposeParDict(const QString& openFoamPath,
                                      int numCores) {
    // Set dictionary content
    QString dictContent = QString(R"(
// Number of cores used for processing
numberOfSubdomains %1;

// Decomposition method
method scotch;
)").arg(QString::number(numCores));

    // Combine everything
    return createFoamHeader("decomposeParDict", openFoamPath) +
           dictContent + createFoamFooter();
}
