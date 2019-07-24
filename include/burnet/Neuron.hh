#ifndef BURNET_NEURON_HH_
#define BURNET_NEURON_HH_

#include "Activation.hh"
#include "Aggregation.hh"

#include <memory>
#include <type_traits>

namespace burnet
{


class INeuron
{
public:
    static void initRandom(unsigned dropCoSeed, double dropCo, unsigned wSeed)
    {
        dropConnect = dropCo;

        if(dropCoSeed == 0)
            dropCoSeed = static_cast<unsigned>(std::chrono::steady_clock().now().time_since_epoch().count());
        dropGen = std::mt19937(dropCoSeed);
        dropSeed = dropCoSeed;

        dropDist = std::bernoulli_distribution(dropCo);

        if(wSeed == 0)
            wSeed = static_cast<unsigned>(std::chrono::steady_clock().now().time_since_epoch().count());
        weightGen = std::mt19937(wSeed);
        weightSeed = wSeed;
    }

protected:
    static unsigned weightSeed;
    static std::mt19937 weightGen;
    static unsigned dropSeed;
    static double dropConnect;
    static std::mt19937 dropGen;
    static std::bernoulli_distribution dropDist;
};

inline unsigned INeuron::weightSeed = 0;
inline std::mt19937 INeuron::weightGen = std::mt19937();
inline unsigned INeuron::dropSeed = 0;
inline double INeuron::dropConnect = 0;
inline std::mt19937 INeuron::dropGen = std::mt19937();
inline std::bernoulli_distribution INeuron::dropDist = std::bernoulli_distribution();


//=============================================================================
//=============================================================================
//=============================================================================
//=== NEURON ==================================================================
//=============================================================================
//=============================================================================
//=============================================================================


template<typename Aggr_t = Dot, typename Act_t = Relu,
typename = typename std::enable_if<
std::is_base_of<Activation, Act_t>::value &&
std::is_base_of<Aggregation, Aggr_t>::value,
void>::type>
class Neuron : public INeuron
{
public:
    Neuron(Aggr_t const& aggregation = Aggr_t(), Act_t const& activation = Act_t(), Matrix const& weights = Matrix(), std::vector<double> const& bias = std::vector<double>()):
    _aggregation(aggregation),
    _activation(activation),
    _weights(weights),
    _bias(bias),
    _inputs(),
    _aggregResults(),
    _actResults(),
    _inputGradients(),
    _actGradients(),
    _gradients(),
    _gradientsPerFeature()
    {
    }


    void init(Distrib distrib, double distVal1, double distVal2, unsigned nbInputs, unsigned nbOutputs, unsigned batchSize, unsigned k)
    {
        _aggregResults = std::vector<std::pair<double, unsigned>>(batchSize, {0.0, 0});
        _actResults = std::vector<double>(batchSize, 0);
        _actGradients = std::vector<double>(batchSize, 0);
        _inputGradients = std::vector<double>(nbOutputs, 0);
        if(_weights.size() == 0)
        {
            _weights = Matrix(k, std::vector<double>(nbInputs, 0));
            _bias = std::vector<double>(k, 0);
        }
        _gradientsPerFeature = Matrix(batchSize, std::vector<double>(_weights[0].size(), 0));
        _gradients = _weights;
        if(distrib == Distrib::Normal)
        {
            double deviation = std::sqrt(distVal2 / (nbInputs + nbOutputs));
            std::normal_distribution<double> normalDist(distVal1, deviation);
            for(unsigned i = 0; i < _weights.size(); i++)
            {
                for(unsigned j = 0; j < _weights[0].size(); j++)
                {
                    _weights[i][j] = normalDist(weightGen);
                }
            }
        }
        else if(distrib == Distrib::Uniform)
        {
            double boundary = std::sqrt(distVal2 / (nbInputs + nbOutputs));
            std::uniform_real_distribution<double> uniformDist(-boundary, boundary);
            for(unsigned i = 0; i < _weights.size(); i++)
            {
                for(unsigned j = 0; j < _weights[0].size(); j++)
                {
                    _weights[i][j] = uniformDist(weightGen);
                }
            }
        }
    }


    //each line of the input matrix is a feature of the batch. Returns one result per feature.
    std::vector<double> process(Matrix const& inputs) const
    {
        std::vector<double> results(inputs.size(), 0);

        for(unsigned i = 0; i < inputs.size(); i++)
        {
            results[i] = _activation.activate(_aggregation.aggregate(inputs[i], _weights, _bias).first);
        }
        return results;
    }


    //each line of the input matrix is a feature of the batch. Returns one result per feature.
    std::vector<double> processToLearn(Matrix const& inputs)
    {
        _inputs = inputs;

        //dropConnect
        if(dropConnect > std::numeric_limits<double>::epsilon())
        {
            for(unsigned i=0; i<_inputs.size(); i++)
            {
                for(unsigned j=0; j<_inputs[0].size(); i++)
                {
                    if(dropDist(dropGen))
                        _inputs[i][j] = 0;
                    else
                        _inputs[i][j] /= (1 - dropConnect);
                }
            }
        }

        //processing
        for(unsigned i = 0; i < inputs.size(); i++)
        {
            _aggregResults[i] = _aggregation.aggregate(inputs[i], _weights, _bias);
            _actResults[i] = _activation.activate(_aggregResults[i].first);
        }

        return _actResults;
    }


    //one input gradient per feature
    void computeGradients(std::vector<double> const& inputGradients)
    {
        _inputGradients = inputGradients;
        std::vector<unsigned> setCount(_weights.size(), 0); //store the amount of feature that passed through each weight set
        for(unsigned feature = 0; feature < _actResults.size(); feature++)
        {
            _actGradients[feature] = _activation.prime(_actResults[feature]) * _inputGradients[feature];
            std::vector<double> grad(_aggregation.prime(_inputs[feature], _weights[_aggregResults[feature].second]));

            for(unsigned i = 0; i < grad.size(); i++)
            {
                _gradients[_aggregResults[feature].second][i] += (_actGradients[feature]*grad[i]);
                _gradientsPerFeature[feature][i] += (_actGradients[feature]* grad[i] * _weights[_aggregResults[feature].second][i]);
            }
            setCount[_aggregResults[feature].second]++;
        }

        //average gradients over features
        for(unsigned i = 0; i < _gradients.size(); i++)
        {
            for(unsigned j = 0; j < _gradients[0].size(); j++)
            {
                _gradients[i][j] /= setCount[i];
            };
        }
    }


    void updateWeights(double learningRate, double L1, double L2, double tackOn, double maxNorm, double momentum)
    {
        double averageInputGrad = 0;
        for(unsigned i = 0; i < _inputGradients.size(); i++)
        {
            averageInputGrad += _inputGradients[i];
        }
        averageInputGrad /= _inputGradients.size();

        double averageActGrad = 0;
        for(unsigned i = 0; i < _actGradients.size(); i++)
        {
            averageActGrad += _actGradients[i];
        }
        averageActGrad /= _actGradients.size();

        _activation.learn(averageInputGrad, learningRate, momentum);
        _aggregation.learn(averageActGrad, learningRate, momentum);

        for(unsigned i = 0; i < _weights.size(); i++)
        {
            for(unsigned j = 0; j < _weights[0].size(); j++)
            {
                _weights[i][j] += (learningRate*(_gradients[i][j] + (L2 * _weights[i][j]) + L1) + tackOn);
                _bias[i] += learningRate * averageActGrad;
            }
        }

        //max norm constraint
        if(maxNorm > 0)
        {
            for(unsigned i = 0; i < _weights.size(); i++)
            {
                double norm = std::sqrt(quadraticSum(_weights[i]));
                if(norm > maxNorm)
                {
                    for(unsigned j=0; j<_weights[i].size(); j++)
                    {
                        _weights[i][j] *= (maxNorm/norm);
                    }
                }
            }
        }
    }


    //one gradient per feature (line) and per input (column)
    Matrix getGradients() const
    {
        return _gradientsPerFeature;
    }


protected:
    Aggr_t _aggregation;
    Act_t _activation;

    Matrix _weights;
    std::vector<double> _bias;

    Matrix _inputs;
    std::vector<std::pair<double, unsigned>> _aggregResults;
    std::vector<double> _actResults;

    std::vector<double> _inputGradients; //gradient from next layer for each feature af the batch
    std::vector<double> _actGradients; //gradient between aggregation and activation
    Matrix _gradients; //sum (over all features of the batch) of partial gradient for each weight
    Matrix _gradientsPerFeature; // store gradients for each feature, summed over weight set

};



} //namespace burnet



#endif //BURNET_NEURON_HH_