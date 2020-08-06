﻿#include "Msnhnet/layers/MsnhRouteLayer.h"

namespace Msnhnet
{
RouteLayer::RouteLayer(const int &batch, std::vector<int> &inputLayerIndexes,
                       std::vector<int> &inputLayerOutputs, const int &groups, const int &groupIndex, const int &addModel)
{
    this->_type              =   LayerType::ROUTE;
    this->_layerName         =   "Route           ";

    this->_batch             =   batch;
    this->_groups            =   groups;
    this->_groupIndex        =   groupIndex;
    this->_addModel          =   addModel;

    int mOutputNum          =   0;

    this->_layerDetail.append("route ");
    char msg[100];

    this->_inputLayerIndexes =   inputLayerIndexes;
    this->_inputLayerOutputs =   inputLayerOutputs;

    for (size_t i = 0; i < inputLayerIndexes.size(); ++i)
    {
#ifdef WIN32
        sprintf_s(msg, " %d", inputLayerIndexes[i]);
#else
        sprintf(msg, " %d", inputLayerIndexes[i]);
#endif
        this->_layerDetail.append(msg);

        if(addModel != 1)
        {
            mOutputNum      =   mOutputNum + inputLayerOutputs[i];
        }
    }

    this->_layerDetail.append("\n");

    if(addModel == 1)
    {
        mOutputNum = inputLayerOutputs[0];
    }

    mOutputNum          =   mOutputNum / groups;
    this->_outputNum     =   mOutputNum;
    this->_inputNum      =   mOutputNum;

    if(!BaseLayer::isPreviewMode)
    {
        this->_output        =   new float[static_cast<size_t>(this->_outputNum*this->_batch)]();
#ifdef USE_GPU
        this->_gpuOutput         = Cuda::makeCudaArray(this->_output, this->_outputNum * this->_batch);
#endif
    }

}

void RouteLayer::forward(NetworkState &netState)        

{
    TimeUtil::startRecord();
    int offset          =   0;
    for (size_t i = 0; i < _inputLayerIndexes.size(); ++i)
    {
        int index       =   this->_inputLayerIndexes[i];
        float *mInput   =   netState.net->layers[static_cast<size_t>(index)]->getOutput();
        int inputLayerOutputs   =   this->_inputLayerOutputs[i];
        int partInSize  =   inputLayerOutputs / this->_groups;
        for (int j = 0; j < this->_batch; ++j)
        {
            if(_addModel == 1)
            {
                Blas::cpuAxpy(partInSize, 1, mInput + j*inputLayerOutputs + partInSize*this->_groupIndex, 1,
                              this->_output + offset + j*this->_outputNum,1);
            }
            else
            {
                Blas::cpuCopy(partInSize, mInput + j*inputLayerOutputs + partInSize*this->_groupIndex, 1,
                              this->_output + offset + j*this->_outputNum,1);
            }
        }
        if(_addModel != 1)
        {
            offset          = offset + partInSize;
        }
    }

    this->_forwardTime =   TimeUtil::getElapsedTime();

}

#ifdef USE_GPU
void RouteLayer::forwardGPU(NetworkState &netState)
{
    this->recordCudaStart();

    int offset          =   0;
    for (size_t i = 0; i < _inputLayerIndexes.size(); ++i)
    {
        int index       =   this->_inputLayerIndexes[i];
        float *mInput   =   netState.net->layers[static_cast<size_t>(index)]->getGpuOutput();
        int inputLayerOutputs   =   this->_inputLayerOutputs[i];
        int partInSize  =   inputLayerOutputs / this->_groups;
        for (int j = 0; j < this->_batch; ++j)
        {
            if(_addModel == 1)
            {
                std::cout<<"================= add model ============================\n";
                BlasGPU::gpuAxpy(partInSize, 1, mInput + j*inputLayerOutputs + partInSize*this->_groupIndex, 1,
                              this->_gpuOutput + offset + j*this->_outputNum,1);
            }
            else
            {
                BlasGPU::gpuCopy(partInSize, mInput + j*inputLayerOutputs + partInSize*this->_groupIndex, 1,
                              this->_gpuOutput + offset + j*this->_outputNum,1);
            }
        }
        if(_addModel != 1)
        {
            offset          = offset + partInSize;
        }
    }

    this->recordCudaStop();
}
#endif

void RouteLayer::resize(Network &net)
{
    BaseLayer first                 =   *net.layers[static_cast<size_t>(this->_inputLayerIndexes[0])];
    this->_outWidth                  =   first.getOutWidth();
    this->_outHeight                 =   first.getOutHeight();
    this->_outChannel                =   first.getOutChannel();
    this->_outputNum                 =   first.getOutputNum();
    this->_inputLayerOutputs[0]      =   first.getOutputNum();

    for (size_t i = 0; i < static_cast<size_t>(this->_num); ++i)
    {
        size_t index                =   static_cast<size_t>(this->_inputLayerIndexes[i]);
        BaseLayer next              =   *net.layers[index];
        this->_outputNum             +=  next.getOutputNum();
        this->_inputLayerOutputs[i]  = next.getOutputNum();

        if(next.getOutWidth() == first.getOutWidth() && next.getOutHeight() == first.getOutHeight())
        {
            this->_outChannel    +=  next.getOutChannel();
        }
        else
        {
            this->_outHeight     =   0;
            this->_outWidth      =   0;
            this->_outChannel    =   0;
            throw Exception(1, "Different size of first layer and secon layer", __FILE__, __LINE__, __FUNCTION__);
        }
    }

    this->_outChannel    =   this->_outChannel/this->_groups;
    this->_outputNum     =   this->_outputNum / this->_groups;
    this->_inputNum      =   this->_outputNum;
    if(this->_output == nullptr)
    {
        throw Exception(1,"output can't be null", __FILE__, __LINE__, __FUNCTION__);
    }

    this->_output    = static_cast<float *>(realloc(this->_output, static_cast<size_t>(this->_outputNum *this->_batch)*sizeof(float)));
}

std::vector<int> RouteLayer::getInputLayerIndexes() const
{
    return _inputLayerIndexes;
}

std::vector<int> RouteLayer::getInputLayerOutputs() const
{
    return _inputLayerOutputs;
}

int RouteLayer::getGroups() const
{
    return _groups;
}

int RouteLayer::getGroupIndex() const
{
    return _groupIndex;
}

int RouteLayer::getAddModel() const
{
    return _addModel;
}
}
