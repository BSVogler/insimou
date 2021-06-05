//
//  numericBackend.cpp
//  insimou
//
//  Created by Benedikt privat on 16.01.21.
//

#include "numericBackend.hpp"
#include <iostream>
#include <random>

NumericBackend::NumericBackend(std::vector<float> min,
                               std::vector<float> max,
                               std::vector<int> res):
    placecelllayer(min, max, res),
    lastmaxindex(-1)
{
    std::random_device rd;
    
    std::mt19937 e2(rd());
    std::uniform_real_distribution<> dist(-0.5, 0.5);
    
    //start with zero input and output
    observation.resize(min.size());//if not 0 wanted use std::fill
    action.resize(5);
    //init with random weights
    int numcells = this->placecelllayer.numCells();
    weight.reserve(numcells);
    std::cout <<"NUM WEIGHTS: "<<numcells <<std::endl;
    for (int i =0; i <numcells ; i++){
        float b = dist(e2);
        weight.push_back(b);
    }
}


void NumericBackend::setObservation(float observation[], int length){
    observationmtx.lock();
    this->observation.resize(length);
    for (int dim = 0; dim < length; dim++){
        this->observation[dim] = observation[dim];
    }
    observationmtx.unlock();
}

void NumericBackend::coreloop(){
    //calling multiple times causes side effects
    
    //only set the last action when loop is once through
    observationmtx.lock();
    if (lastmaxindex > -1){
        lastaction = observation.at(lastmaxindex)* weight.at(lastmaxindex);//action is det. by weight
    }
    
    //get activation of all input layer neurons
    auto activations = this->placecelllayer.activation(this->observation);
    observationmtx.unlock();
    //lateral inhibition causes one hot encoding, find maximum
    lastmaxindex = int(std::distance(activations.begin(), std::max_element(activations.begin(), activations.end())));
    
    this->lastactivation = this->weight[lastmaxindex];
    
    //todo only works on pole balancing
    //rate should only be a scalar value
    this->action[0] = int(copysign(1.0, (float)(this->lastactivation)) == 1); // 0 or 1
}

void NumericBackend::setFeedback(float errsig){
    //update by adding the error
    if (lastmaxindex>0){
        auto neww = weight.at(lastmaxindex)+float(copysign(1.0, (float)(lastaction)) * errsig * learningrate);
        //limit
        neww = std::max(-gvwmax, std::min(neww, gvwmax));
        weight.at(lastmaxindex) = neww;
    }
}

float* NumericBackend::getWeights(){
    return weight.data();
}

float* NumericBackend::getActions(){
    //todo block here until other thread is ready
    return action.data();
}