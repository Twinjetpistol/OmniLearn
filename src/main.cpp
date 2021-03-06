
#include "omnilearn/Network.hh"


void vesta()
{
    omnilearn::Data data = omnilearn::loadData("dataset/vesta.csv", ';', 4);

    omnilearn::NetworkParam netp;
    netp.threads = 4;
    netp.batchSize = 10;
    netp.learningRate = 0.001;
    netp.loss = omnilearn::Loss::L2;
    netp.patience = 10;
    netp.plateau = 0.99;
    netp.decay = omnilearn::Decay::Plateau;
    netp.decayValue = 2;
    netp.decayDelay = 2;
    netp.validationRatio = 0.15;
    netp.testRatio = 0.15;
    netp.optimizer = omnilearn::Optimizer::Rmsprop;
    netp.preprocessInputs = {omnilearn::Preprocess::Center, omnilearn::Preprocess::Decorrelate, omnilearn::Preprocess::Whiten};
    netp.preprocessOutputs = {omnilearn::Preprocess::Center, omnilearn::Preprocess::Decorrelate, omnilearn::Preprocess::Normalize};

    omnilearn::Network net(data, netp);

    omnilearn::LayerParam lay;
    lay.maxNorm = 5;
    lay.size = 32;
    net.addLayer(lay, omnilearn::Aggregation::Dot, omnilearn::Activation::Relu);
    net.addLayer(lay, omnilearn::Aggregation::Dot, omnilearn::Activation::Linear);


    net.learn();
}


void mnist()
{
    omnilearn::Data data = omnilearn::loadData("dataset/mnist_train.csv", ',', 4);
    omnilearn::Data testdata = omnilearn::loadData("dataset/mnist_test.csv", ',', 4);

    omnilearn::NetworkParam netp;
    netp.threads = 4;
    netp.batchSize = 100;
    netp.learningRate = 0.0002;
    netp.loss = omnilearn::Loss::CrossEntropy;
    netp.epoch = 500;
    netp.patience = 10;
    netp.plateau = 0.99;
    netp.decay = omnilearn::Decay::Plateau;
    netp.decayValue = 2;
    netp.decayDelay = 2;
    netp.classValidity = 0.80;
    netp.validationRatio = 0.15;
    netp.testRatio = 0.0;
    netp.optimizer = omnilearn::Optimizer::Rmsprop;
    netp.preprocessInputs = {omnilearn::Preprocess::Center, omnilearn::Preprocess::Decorrelate, omnilearn::Preprocess::Reduce};
    netp.preprocessOutputs = {};
    netp.inputReductionThreshold = 0.99;

    omnilearn::Network net(data, netp);
    net.setTestData(testdata);

    omnilearn::LayerParam lay;
    lay.maxNorm = 5;
    lay.size = 300;
    net.addLayer(lay, omnilearn::Aggregation::Dot, omnilearn::Activation::Relu);
    net.addLayer(lay, omnilearn::Aggregation::Dot, omnilearn::Activation::Linear);

    net.learn();
}


void testLoader()
{
    omnilearn::Network genNet("omnilearn_network", 4);
    omnilearn::Data data = omnilearn::loadData("dataset/mnist_test.csv", ',', 4);
    std::pair<double, double> metric = omnilearn::classificationMetrics(data.outputs, genNet.process(data.inputs), 0.8);
    std::cout << metric.first << " " << metric.second << "\n";
}


int main()
{
    //mnist();
    //vesta();
    testLoader();

    return 0;
}