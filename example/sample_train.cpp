/*
    Copyright (c) 2013, Taiga Nomi
    All rights reserved.
    
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY 
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <iostream>

#include "tiny_cnn.h"
//#include "imdebug.h"

void sample1_convnet(std::string data_dir_path);
void sample2_mlp(std::string data_dir_path);
void sample3_dae( );
void sample4_dropout(std::string data_dir_path);
void sample5_convnet_ghh(std::string data_dir_path);

using namespace tiny_cnn;
using namespace tiny_cnn::activation;

int main(int argc,char **argv) {
    if (argc!=2){
        std::cerr<<"Usage : "<<argv[0]<<" path_to_data \n(example:"<<argv[0]<<" ../data)"<<std::endl;
        return -1;
    }
    // sample1_convnet(argv[1]);
    sample5_convnet_ghh(argv[1]);
}

///////////////////////////////////////////////////////////////////////////////
// learning convolutional neural networks (LeNet-5 like architecture)
void sample1_convnet(std::string data_dir_path) {
    // construct LeNet-5 architecture
    network<mse, gradient_descent_levenberg_marquardt> nn;

    // connection table [Y.Lecun, 1998 Table.1]
#define O true
#define X false
    static const bool connection [] = {
        O, X, X, X, O, O, O, X, X, O, O, O, O, X, O, O,
        O, O, X, X, X, O, O, O, X, X, O, O, O, O, X, O,
        O, O, O, X, X, X, O, O, O, X, X, O, X, O, O, O,
        X, O, O, O, X, X, O, O, O, O, X, X, O, X, O, O,
        X, X, O, O, O, X, X, O, O, O, O, X, O, O, X, O,
        X, X, X, O, O, O, X, X, O, O, O, O, X, O, O, O
    };
#undef O
#undef X

    nn << convolutional_layer<tan_h>(32, 32, 5, 1, 6) // 32x32 in, 5x5 kernel, 1-6 fmaps conv
       << average_pooling_layer<tan_h>(28, 28, 6, 2) // 28x28 in, 6 fmaps, 2x2 subsampling
       << convolutional_layer<tan_h>(14, 14, 5, 6, 16,connection_table(connection, 6, 16)) // with connection-table
       << average_pooling_layer<tan_h>(10, 10, 16, 2)
       << convolutional_layer<tan_h>(5, 5, 5, 16, 120)
       << fully_connected_layer<tan_h>(120, 10);
 
    std::cout << "load models..." << std::endl;

    // load MNIST dataset
    std::vector<label_t> train_labels, test_labels;
    std::vector<vec_t> train_images, test_images;

    parse_mnist_labels(data_dir_path+"/train-labels.idx1-ubyte", &train_labels);
    parse_mnist_images(data_dir_path+"/train-images.idx3-ubyte", &train_images, -1.0, 1.0, 2, 2);
    parse_mnist_labels(data_dir_path+"/t10k-labels.idx1-ubyte", &test_labels);
    parse_mnist_images(data_dir_path+"/t10k-images.idx3-ubyte", &test_images, -1.0, 1.0, 2, 2);

    std::cout << "start learning" << std::endl;

    tiny_cnn::progress_display disp(train_images.size());
    tiny_cnn::timer t;
    int minibatch_size = 10;

    nn.optimizer().alpha *= std::sqrt(minibatch_size);

    // create callback
    auto on_enumerate_epoch = [&](){
        std::cout << t.elapsed() << "s elapsed." << std::endl;

        tiny_cnn::result res = nn.test(test_images, test_labels);

        std::cout << nn.optimizer().alpha << "," << res.num_success << "/" << res.num_total << std::endl;

        nn.optimizer().alpha *= 0.85; // decay learning rate
        nn.optimizer().alpha = std::max((float_t)(0.00001), nn.optimizer().alpha);

        disp.restart(train_images.size());
        t.restart();
    };

    auto on_enumerate_minibatch = [&](){ 
        disp += minibatch_size; 
    
        // weight visualization in imdebug
        /*static int n = 0;    
        n+=minibatch_size;
        if (n >= 1000) {
            image img;
            C3.weight_to_image(img);
            imdebug("lum b=8 w=%d h=%d %p", img.width(), img.height(), &img.data()[0]);
            n = 0;
        }*/
    };
    
    // training
    nn.train(train_images, train_labels, minibatch_size, 20, on_enumerate_minibatch, on_enumerate_epoch);

    std::cout << "end training." << std::endl;

    // test and show results
    nn.test(test_images, test_labels).print_detail(std::cout);

    // save networks
    std::ofstream ofs("LeNet-weights");
    ofs << nn;
}


///////////////////////////////////////////////////////////////////////////////
// learning 3-Layer Networks
void sample2_mlp(std::string data_dir_path)
{
    const int num_hidden_units = 500;

#if defined(_MSC_VER) && _MSC_VER < 1800
    // initializer-list is not supported
    int num_units[] = { 28 * 28, num_hidden_units, 10 };
    auto nn = make_mlp<mse, gradient_descent, tan_h>(num_units, num_units + 3);
#else
    auto nn = make_mlp<mse, gradient_descent, tan_h>({ 28 * 28, num_hidden_units, 10 });
#endif

    // load MNIST dataset
    std::vector<label_t> train_labels, test_labels;
    std::vector<vec_t> train_images, test_images;

    parse_mnist_labels(data_dir_path+"/train-labels.idx1-ubyte", &train_labels);
    parse_mnist_images(data_dir_path+"/train-images.idx3-ubyte", &train_images, -1.0, 1.0, 0, 0);
    parse_mnist_labels(data_dir_path+"/t10k-labels.idx1-ubyte", &test_labels);
    parse_mnist_images(data_dir_path+"/t10k-images.idx3-ubyte", &test_images, -1.0, 1.0, 0, 0);

    nn.optimizer().alpha = 0.001;
    
    tiny_cnn::progress_display disp(train_images.size());
    tiny_cnn::timer t;

    // create callback
    auto on_enumerate_epoch = [&](){
        std::cout << t.elapsed() << "s elapsed." << std::endl;

        tiny_cnn::result res = nn.test(test_images, test_labels);

        std::cout << nn.optimizer().alpha << "," << res.num_success << "/" << res.num_total << std::endl;

        nn.optimizer().alpha *= 0.85; // decay learning rate
        nn.optimizer().alpha = std::max((float_t)(0.00001), nn.optimizer().alpha);

        disp.restart(train_images.size());
        t.restart();
    };

    auto on_enumerate_data = [&](){ 
        ++disp; 
    };  

    nn.train(train_images, train_labels, 1, 20, on_enumerate_data, on_enumerate_epoch);
}

///////////////////////////////////////////////////////////////////////////////
// denoising auto-encoder
void sample3_dae()
{
#if defined(_MSC_VER) && _MSC_VER < 1800
    // initializer-list is not supported
    int num_units[] = { 100, 400, 100 };
    auto nn = make_mlp<mse, gradient_descent, tan_h>(num_units, num_units + 3);
#else
    auto nn = make_mlp<mse, gradient_descent, tan_h>({ 100, 400, 100 });
#endif

    std::vector<vec_t> train_data_original;

    // load train-data

    std::vector<vec_t> train_data_corrupted(train_data_original);

    for (auto& d : train_data_corrupted) {
        d = corrupt(move(d), 0.1, 0.0); // corrupt 10% data
    }

    // learning 100-400-100 denoising auto-encoder
    nn.train(train_data_corrupted, train_data_original);
}

///////////////////////////////////////////////////////////////////////////////
// dropout-learning

void sample4_dropout(std::string data_dir_path)
{
    typedef network<mse, gradient_descent> Network;
    Network nn;
    int input_dim    = 28*28;
    int hidden_units = 800;
    int output_dim   = 10;

    fully_connected_dropout_layer<tan_h> f1(input_dim, hidden_units, dropout::per_data);
    fully_connected_layer<tan_h> f2(hidden_units, output_dim);
    nn << f1 << f2;

    nn.optimizer().alpha = 0.003; // TODO: not optimized
    nn.optimizer().lambda = 0.0;

    // load MNIST dataset
    std::vector<label_t> train_labels, test_labels;
    std::vector<vec_t> train_images, test_images;

    parse_mnist_labels(data_dir_path+"/train-labels.idx1-ubyte", &train_labels);
    parse_mnist_images(data_dir_path+"/train-images.idx3-ubyte", &train_images, -1.0, 1.0, 0, 0);
    parse_mnist_labels(data_dir_path+"/t10k-labels.idx1-ubyte", &test_labels);
    parse_mnist_images(data_dir_path+"/t10k-images.idx3-ubyte", &test_images, -1.0, 1.0, 0, 0);

    // load train-data, label_data
    tiny_cnn::progress_display disp(train_images.size());
    tiny_cnn::timer t;

    // create callback
    auto on_enumerate_epoch = [&](){
        std::cout << t.elapsed() << "s elapsed." << std::endl;

        f1.set_context(dropout::test_phase);
        tiny_cnn::result res = nn.test(test_images, test_labels);
        f1.set_context(dropout::train_phase);


        std::cout << nn.optimizer().alpha << "," << res.num_success << "/" << res.num_total << std::endl;

        nn.optimizer().alpha *= 0.99; // decay learning rate
        nn.optimizer().alpha = std::max((float_t)(0.00001), nn.optimizer().alpha);

        disp.restart(train_images.size());
        t.restart();
    };

    auto on_enumerate_data = [&](){
        ++disp;
    };

    nn.train(train_images, train_labels, 1, 100, on_enumerate_data, on_enumerate_epoch);

    // change context to enable all hidden-units
    //f1.set_context(dropout::test_phase);
    //std::cout << res.num_success << "/" << res.num_total << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
// learning convolutional neural networks (LeNet-5 like architecture)
// using ghh_activation
void sample5_convnet_ghh(std::string data_dir_path) {
    // construct LeNet-5 architecture
    // network<mse, adam> nn;
    network<cross_entropy_multiclass, adam> nn;

    // connection table [Y.Lecun, 1998 Table.1]
// #define O true
// #define X false
//     static const bool connection [] = {
//         O, X, X, X, O, O, O, X, X, O, O, O, O, X, O, O,
//         O, O, X, X, X, O, O, O, X, X, O, O, O, O, X, O,
//         O, O, O, X, X, X, O, O, O, X, X, O, X, O, O, O,
//         X, O, O, O, X, X, O, O, O, O, X, X, O, X, O, O,
//         X, X, O, O, O, X, X, O, O, O, O, X, O, O, X, O,
//         X, X, X, O, O, O, X, X, O, O, O, O, X, O, O, O
//     };
// #undef O
// #undef X

	convolutional_layer<relu> conv1(32, 32, 5, 1, 6); // 32x32 in, 5x5 kernel, 1-6 fmaps conv
	max_pooling_layer<identity> maxpool1(28, 28, 6, 2);		  // 28x28 in, 6 fmaps, 2x2 subsampling

	convolutional_layer<relu> conv2(14, 14, 5, 6, 16);
	max_pooling_layer<identity> maxpool2(10, 10, 16, 2);

	convolutional_layer<relu> conv3(5, 5, 5, 16, 100);

	fully_connected_layer<identity> fc(100, 160); // fully connected

	//------------ testing different activations for the last  FC layer -----------
	// fully_connected_layer<relu> out_layer(160, 10); // ReLU activation
	
	ghh_activation_layer<identity> out_layer(10,4,4);   // ghh activation without dropout
	
	// ghh_activation_dropout_layer<identity> out_layer(10,4,4);   // ghh activation with dropout
	// fc1_ghh.set_dropout_rate(0.3);
	//------------------------------------------------------------------------------

	max_pooling_layer<softmax> softmax_layer(1, 1, 10, 1); // just to do soft max at the end to form as multiclass problem
	
    nn << conv1 << maxpool1 << conv2 << maxpool2 << conv3 << fc << out_layer << softmax_layer;

	
    std::cout << "load models..." << std::endl;

    // load MNIST dataset
    std::vector<label_t> train_labels, test_labels;
    std::vector<vec_t> train_images, test_images;

    parse_mnist_labels(data_dir_path+"/train-labels.idx1-ubyte", &train_labels);
    parse_mnist_images(data_dir_path+"/train-images.idx3-ubyte", &train_images, -1.0, 1.0, 2, 2);
    parse_mnist_labels(data_dir_path+"/t10k-labels.idx1-ubyte", &test_labels);
    parse_mnist_images(data_dir_path+"/t10k-images.idx3-ubyte", &test_images, -1.0, 1.0, 2, 2);

    std::cout << "start learning" << std::endl;

    tiny_cnn::progress_display disp(train_images.size());
    tiny_cnn::timer t;
    int minibatch_size = 10;

    nn.optimizer().alpha *= std::sqrt(minibatch_size);

    // create callback
    auto on_enumerate_epoch = [&](){
        std::cout << t.elapsed() << "s elapsed." << std::endl;

        tiny_cnn::result res = nn.test(test_images, test_labels);
        tiny_cnn::result res_train = nn.test(train_images, train_labels);

        std::cout << nn.optimizer().alpha << ",\t" << res.num_success << "/" << res.num_total
		<< ",\t"<< res_train.num_success << "/" << res_train.num_total << std::endl;

        nn.optimizer().alpha *= 0.85; // decay learning rate
        nn.optimizer().alpha = std::max((float_t)(0.00001), nn.optimizer().alpha);

        disp.restart(train_images.size());
        t.restart();
    };

    auto on_enumerate_minibatch = [&](){ 
        disp += minibatch_size; 
    
        // weight visualization in imdebug
        /*static int n = 0;    
        n+=minibatch_size;
        if (n >= 1000) {
            image img;
            C3.weight_to_image(img);
            imdebug("lum b=8 w=%d h=%d %p", img.width(), img.height(), &img.data()[0]);
            n = 0;
        }*/
    };
    
    // training
    nn.train(train_images, train_labels, minibatch_size, 20, on_enumerate_minibatch, on_enumerate_epoch);

    std::cout << "end training." << std::endl;

    // test and show results
    nn.test(test_images, test_labels).print_detail(std::cout);

    // save networks
    std::ofstream ofs("LeNet-weights");
    ofs << nn;
}

