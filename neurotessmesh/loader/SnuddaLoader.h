//
// Created by gaeqs on 16/03/25.
//

#ifndef SNUDDALOADER_H
#define SNUDDALOADER_H

#include <nsol/DataSet.h>
#include <boost/filesystem/path.hpp>

#include <H5Cpp.h>

namespace neurotessmesh {
    class SnuddaLoader {
        std::unique_ptr<H5::H5File> _file;
        boost::filesystem::path _dataPath;
        size_t _maxNeurons;

        std::unordered_map<uint32_t, nsol::Neuron*> loadNeurons(nsol::DataSet& dataset) const;

        std::string loadMorphologies(const std::unordered_map<uint32_t, nsol::Neuron*>& neurons) const;

    public:
        explicit SnuddaLoader(const boost::filesystem::path& path, const boost::filesystem::path &repoPath);

        ~SnuddaLoader();

        void setMaxNeuronsToLoad(size_t maxNeurons);

        void load(nsol::DataSet& dataset) const;
    };
}


#endif //SNUDDALOADER_H
