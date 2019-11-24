

#define BRAIN_ENABLE_SVD
#include "brain/brain.hh"


int main()
{
    brain::Data data = brain::loadData("dataset/mnist_train.csv", ',', 4);
    brain::Data testdata = brain::loadData("dataset/mnist_test.csv", ',', 4);

    brain::NetworkParam netp;
    netp.threads = 4;
    netp.batchSize = 100;
    netp.learningRate = 0.0001;
    netp.loss = brain::Loss::CrossEntropy;
    netp.epoch = 500;
    netp.patience = 5;
    netp.plateau = 0.99;
    netp.decay = brain::Decay::Plateau;
    netp.decayValue = 2;
    netp.decayDelay = 2;
    netp.classValidity = 0.80;
    netp.validationRatio = 0.15;
    netp.testRatio = 0.00;
    netp.optimizer = brain::Optimizer::Rmsprop;
    netp.preprocessInputs = {brain::Preprocess::Center, brain::Preprocess::Decorrelate, brain::Preprocess::Reduce};
    //netp.preprocessOutputs = {brain::Preprocess::Center, brain::Preprocess::Decorrelate, brain::Preprocess::Reduce, brain::Preprocess::Normalize};
    netp.optimizerBias = 1e-5;
    netp.outputReductionThreshold = 0.95;

    brain::Network net(data, netp);
    net.setTestData(testdata);

    brain::LayerParam lay;
    lay.maxNorm = 5;
    lay.size = 100;
    net.addLayer<brain::Dot, brain::Relu>(lay);
    net.addLayer<brain::Dot, brain::Linear>(lay);


    if(net.learn())
    {
        net.writeInfo("output.out");
    }

    return 0;
}