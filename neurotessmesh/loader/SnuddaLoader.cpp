//
// Created by gaeqs on 16/03/25.
//

#include "SnuddaLoader.h"

#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

namespace neurotessmesh {
    std::unordered_map<uint32_t, nsol::Neuron*> SnuddaLoader::loadNeurons(nsol::DataSet& dataset) const {
        auto idsDS = _file->openDataSet("network/neurons/neuron_id");
        auto positionDS = _file->openDataSet("network/neurons/position");
        auto rotationDS = _file->openDataSet("network/neurons/rotation");

        hsize_t dims;
        idsDS.getSpace().getSimpleExtentDims(&dims);

        std::vector<uint32_t> ids(dims);
        std::vector<std::array<double, 3>> position(dims);
        std::vector<std::array<double, 9>> rotation(dims);

        idsDS.read(ids.data(), H5::PredType::INTEL_I32);
        positionDS.read(position.data(), H5::PredType::IEEE_F64LE);
        rotationDS.read(rotation.data(), H5::PredType::IEEE_F64LE);

        std::unordered_map<uint32_t, nsol::Neuron*> neurons;

        for (size_t i = 0; i < std::min(ids.size(), _maxNeurons); ++i) {
            glm::vec3 pos(position[i][0], position[i][1], position[i][2]);

            Eigen::Matrix4f model;
            for (size_t c = 0; c < 3; ++c) {
                for (size_t r = 0; r < 3; ++r) {
                    model(c, r) = static_cast<float>(rotation[i][c + r * 3]);
                }
            }
            model(3, 0) = pos[0];
            model(3, 1) = pos[1];
            model(3, 2) = pos[2];
            model(3, 3) = 1.0f;

            nsol::Neuron* neuron = new nsol::Neuron(nullptr, 0, ids[i], model,
                                                    nullptr, nsol::Neuron::PYRAMIDAL);
            dataset.addNeuron(neuron);
            neurons.insert({ids[i], neuron});
        }

        idsDS.close();
        positionDS.close();
        rotationDS.close();

        return neurons;
    }

    std::string SnuddaLoader::loadMorphologies(const std::unordered_map<uint32_t, nsol::Neuron*>& neurons) const {
        static const std::string SNUDDA_PREFIX = "$SNUDDA_DATA";

        auto morphologiesDS = _file->openDataSet("network/neurons/morphology");
        auto stringType = morphologiesDS.getStrType();

        hsize_t dims;
        morphologiesDS.getSpace().getSimpleExtentDims(&dims);

        std::vector<char*> cStringArray(dims, nullptr);
        H5::DataSpace dataspace = morphologiesDS.getSpace();
        morphologiesDS.read(cStringArray.data(), stringType, dataspace);

        std::unordered_map<std::string, nsol::NeuronMorphologyPtr> loaded;

        nsol::SwcReaderTemplated<nsol::Node, nsol::NeuronMorphologySection,
            nsol::Dendrite, nsol::Axon, nsol::Soma,
            nsol::NeuronMorphology, nsol::Neuron> reader;

        for (auto& pair: neurons) {
            auto id = pair.first;
            auto neuron = pair.second;
            auto name = std::string(cStringArray[id]);
            if (loaded.count(name) > 0) {
                auto morphology = loaded[name];
                neuron->morphology(morphology);
                morphology->parentNeurons().push_back(neuron);
                continue;
            }
            std::string modified = name;
            modified.replace(0, SNUDDA_PREFIX.length(), _dataPath.string());
            std::cout << modified << std::endl;
            auto morphology = reader.readMorphology(modified, false);
            loaded[name] = morphology;
            neuron->morphology(morphology);
            morphology->parentNeurons().push_back(neuron);
        }

        morphologiesDS.close();

        return {};
    }

    void SnuddaLoader::setMaxNeuronsToLoad(size_t maxNeurons) {
        _maxNeurons = maxNeurons;
    }

    SnuddaLoader::SnuddaLoader(const boost::filesystem::path& path)
        : _file(std::make_unique<H5::H5File>(path.string(), 0)),
          _dataPath(path.parent_path() / "data"),
          _maxNeurons(10000000) {}

    SnuddaLoader::~SnuddaLoader() {
        _file->close();
    }


    void SnuddaLoader::load(nsol::DataSet& dataset) const {
        auto neurons = loadNeurons(dataset);

        auto error = loadMorphologies(neurons);
        if (!error.empty()) {
            std::cerr << error << std::endl;
        }
    }
}
