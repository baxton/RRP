

// g++ -O3 -I. ann_red.cpp -shared -o libredann.so
// g++ -O3 -I. ann_red.cpp -shared -o redann.dll


#if !defined ANN_DOT_HPP
#define ANN_DOT_HPP


#include <cmath>
#include <iostream>
#include <vector>
#include <iomanip>
#include <algorithm>

#include <memory.hpp>
#include <linalg.hpp>
#include <random.hpp>



using namespace std;


namespace ma {


template<class T>
struct IDecision {
    virtual T delta(int id) = 0;
};

template <class T>
class ann_leaner {
    vector<int> sizes_;

    memory::ptr_vec<T> aa_;
    memory::ptr_vec<T> ww_;
    memory::ptr_vec<T> bb_;

    memory::ptr_vec<T> deltas_;

    memory::ptr_vec<T> bb_deriv_;
    memory::ptr_vec<T> ww_deriv_;

    int total_bb_size_;
    int total_ww_size_;
    int total_aa_size_;

    bool regres_;

    memory::ptr_vec<T> ww_mem_;
    memory::ptr_vec<T> bb_mem_;

    IDecision<T>* pdec_;
    int dec_id_;

public:
    ann_leaner(const vector<int>& sizes, bool regres, IDecision<T>* pdec, int dec_id) :
        sizes_(sizes),
        aa_(),
        ww_(),
        bb_(),
        deltas_(),
        bb_deriv_(),
        ww_deriv_(),
        total_bb_size_(0),
        total_ww_size_(0),
        total_aa_size_(0),
        regres_(regres),
        ww_mem_(),
        bb_mem_(),
        pdec_(pdec),
        dec_id_(dec_id)
    {
        int layers_num = sizes_.size();

        // calculate sizes for arrays
        // I skip level 0 as it's the input vector x
        for (int l = 1; l < layers_num; ++l) {
            int l_layer_size = sizes_[layers_num - l];

            total_bb_size_ += l_layer_size;
            total_aa_size_ += l_layer_size;
            total_ww_size_ += (l_layer_size * sizes_[layers_num - l - 1]);
        }

        // alloacate vectors
        aa_.reset(new T[total_aa_size_]);
        bb_.reset(new T[total_bb_size_]);
        ww_.reset(new T[total_ww_size_]);

        bb_mem_.reset(new T[total_bb_size_]);
        ww_mem_.reset(new T[total_ww_size_]);

        deltas_.reset(new T[total_aa_size_]);

        // partial derivatives
        bb_deriv_.reset(new T[total_bb_size_]);
        ww_deriv_.reset(new T[total_ww_size_]);


        // initialise biases and weights with random values
//        random::rand<T>(bb_.get(), total_bb_size_);
//        random::rand<T>(ww_.get(), total_ww_size_);
        random::randn<T>(bb_.get(), total_bb_size_, 0., .02);
        random::randn<T>(ww_.get(), total_ww_size_, 0., .02);

/*
        for (int i = 0; i < total_bb_size_; ++i)
            bb_[i] = (bb_[i] - .5) / 15;

        int ww_idx = 0;
        for (int l = 1; l < layers_num; ++l) {
            int size = sizes_[l] * sizes_[l-1];
            for (int i = 0; i < size; ++i) {
                ww_[ww_idx + i] = (ww_[ww_idx + i] - .5) / 15;
            }
            ww_idx += size;
        }
*/
    }

    void save_weights() {
        linalg::copy(ww_mem_.get(), ww_.get(), total_ww_size_);
        linalg::copy(bb_mem_.get(), bb_.get(), total_bb_size_);
    }

    void restore_weights() {
        linalg::copy(ww_.get(), ww_mem_.get(), total_ww_size_);
        linalg::copy(bb_.get(), bb_mem_.get(), total_bb_size_);
    }


    void random_shift() {
        for (int i = 0; i < total_ww_size_; ++i) {
            int tmp = random::randint(0, 10);
            if (tmp <= 5) {
                T tmp;
                random::randn(&tmp, 1, 0., .5);
                ww_[i] += tmp;
            }
        }
        for (int i = 0; i < total_bb_size_; ++i) {
            int tmp = random::randint(0, 10);
            if (tmp <= 5) {
                T tmp;
                random::randn(&tmp, 1, 0., .5);
                bb_[i] += tmp;
            }
        }
    }


    template <class I>
    void print_vector(ostream& os, const I* vec, int size, const char* comment, const char* name) {
        os << "// " << comment << endl;
        os << "int " << name << "_size = " << size << ";" << endl;
        os << "int " << name << "[] = {";
        for (int i = 0; i < size; ++i) {
            os << setprecision(16) << vec[i] << ",";
        }
        os << "};" << endl;
    }

    void print(ostream& os) {
        int layers_num = sizes_.size();

        print_vector(os, &sizes_[0], sizes_.size(), "ANN", "sizes");
        print_vector(os, bb_.get(), total_bb_size_, "biases", "bb");
        print_vector(os, ww_.get(), total_ww_size_, "weights", "ww");
        //print_vector(os, bb_deriv_.get(), total_bb_size_, "bb derivs", "bb_deriv");
        //print_vector(os, ww_deriv_.get(), total_ww_size_, "ww derivs", "ww_deriv");

    }

    // calculates sigmoid in place
    void sigmoid(T* v, int size, T m = 1.) {
        for (int i = 0; i < size; ++i) {
            v[i] = 1. / (1. + ::exp(-v[i] * m));
/*
            if (isnan(v[i]))
                v[i] = 0.00000000000001;

            if (isinf(v[i]))
                v[i] = .999999999999;
*/
        }
    }


    void exp(T* v, int size) {
        for (int i = 0; i < size; ++i) {
            v[i] = ::exp(v[i]);
        }
    }


    void tangh(T* v, int size) {
        for (int i = 0; i < size; ++i) {
            T tmp = 2. * v[i];
            v[i] = (1. - ::exp(-tmp)) / (1. + ::exp(tmp));

            if (isnan(v[i]))
                v[i] = 0.00000000000001;

            if (isinf(v[i]))
                v[i] = .999999999999;

        }
    }

    void softmax(T* v, int size) {
        T sum_exp = 0;
        for (int i = 0; i < size; ++i) {
            T tmp = ::exp(v[i]);
/*
            if (isnan(tmp))
                tmp = 0.00000000000001;
            if (isinf(v[i]))
                tmp = .999999999999;
*/
            sum_exp += tmp;
        }

        for (int i = 0; i < size; ++i) {
            v[i] = ::exp(v[i]) / sum_exp;
/*
            if (isnan(v[i]))
                v[i] = 0.00000000000001;
            if (isinf(v[i]))
                v[i] = .999999999999;
*/
        }
    }


    double logloss(double yhat, double y) {
        return -1. * (y * ::log( yhat > 0. ? yhat : 0.00000000000001 ) + (1. - y) * ::log((1. - yhat) > 0. ? (1. - yhat) : 0.00000000000001 ));
    }


    void reset_deriv_for_next_minibatch() {
        linalg::fill(bb_deriv_.get(), total_bb_size_, (T)0);
        linalg::fill(ww_deriv_.get(), total_ww_size_, (T)0);
    }




    // the size of x is sizes_[0]
    void forward(const T* x) {
        int layers_num = sizes_.size();

        const T* S = x;

        int aa_idx = 0;
        int bb_idx = 0;
        int ww_idx = 0;

        for (int l = 1; l < layers_num; ++l) {
            linalg::dot_m2v(&ww_[ww_idx], S, &aa_[aa_idx], sizes_[l], sizes_[l - 1]);
            linalg::sum_v2v(&aa_[aa_idx], &bb_[bb_idx], sizes_[l]);

            if (l == (layers_num - 1)) {
                if (!regres_)
                    softmax(&aa_[aa_idx], sizes_[l]);
                    //sigmoid(&aa_[aa_idx], sizes_[l], 1.);
                // else linear


            }
            else {
                //tangh(&aa_[aa_idx], sizes_[l]);
                //sigmoid(&aa_[aa_idx], sizes_[l], 1.);
            }

            S = &aa_[aa_idx];

            aa_idx += sizes_[l];
            bb_idx += sizes_[l];
            ww_idx += (sizes_[l] * sizes_[l - 1]);
        }
    }

    T backward(const T* x, const T* y) {

        // accumulator for cost
        T cost = 0;

        //
        int aa_idx = total_aa_size_;
        int ww_idx = total_ww_size_;

        int layers_num = sizes_.size();

        for (int l = 1; l < layers_num; ++l) {
            int l_idx = layers_num - l;

            // define shifts in the buffers
            aa_idx -= sizes_[l_idx];

            if (1 == l) {
                // last layer

                // calculate derivatives
                for (int a = 0; a < sizes_[l_idx]; ++a) {
                    //if (regres_)
                        cost += (aa_[aa_idx + a] - y[a]) * (aa_[aa_idx + a] - y[a]);
                    //else
                    //    cost += logloss(aa_[aa_idx + a], y[a]);


                    T delta = pdec_ ? pdec_->delta(dec_id_) : (aa_[aa_idx + a] - y[a]);
                    deltas_[aa_idx + a] = delta;
                    bb_deriv_[aa_idx + a] += delta;
                }

                cost /= 2.;
                cost /= sizes_[l_idx];
            }
            else {
                // hidden layers

                ww_idx -= (sizes_[l_idx + 1] * sizes_[l_idx]);
                int aa_idx_next = aa_idx + sizes_[l_idx];

                // calculate derivatives
                for (int a = 0; a < sizes_[l_idx]; ++a) {

                    T delta = 0;
                    for (int a_next = 0; a_next < sizes_[l_idx + 1]; ++a_next) {
                        delta += deltas_[aa_idx_next + a_next] * ww_[ww_idx + a_next * sizes_[l_idx] + a];
                    }

                    //T sig_deriv = aa_[aa_idx + a] * (1. - aa_[aa_idx + a]);
                    //delta *= sig_deriv;

                    deltas_[aa_idx + a] = delta;
                    bb_deriv_[aa_idx + a] += delta;
                }
            }
        }

        // grad
        int aa_idx_next = 0;
        ww_idx = 0;

        const T* Z = x;

        for (int l = 0; l < layers_num-1; ++l) {

            for (int a = 0; a < sizes_[l]; ++a) {
                for (int a_next = 0; a_next < sizes_[l+1]; ++a_next) {
                    ww_deriv_[ww_idx + a_next * sizes_[l] + a] += deltas_[aa_idx_next + a_next] * Z[a];
                }
            }

            // next
            Z = &aa_[aa_idx_next];

            aa_idx_next += sizes_[l+1];
            ww_idx += sizes_[l] * sizes_[l+1];
        }

        return cost;
    }

    void average_deriv(T sample_size) {
        linalg::div_v2s(bb_deriv_.get(), total_bb_size_, sample_size);
        linalg::div_v2s(ww_deriv_.get(), total_ww_size_, sample_size);
    }



    T fit_minibatch(const T* X, const T* Y, int rows, T alpha, T lambda) {

        reset_deriv_for_next_minibatch();

        int x_columns_num = sizes_[0];
        int y_columns_num = sizes_[ sizes_.size() - 1 ];

        T cost = (T)0;

        for (int r = 0; r < rows; ++r) {
            forward(&X[r * x_columns_num]);
            cost += backward(&X[r * x_columns_num], &Y[r * y_columns_num]);
        }

        average_deriv((T)rows);

        T reg = regularize(lambda, rows);
/*
        for (int w = 0; w < total_ww_size_; ++w) {
            reg += lambda * ww_[w] * ww_[w] / (2. * rows);
            ww_deriv_[w] += lambda * ww_[w] / rows;
        }
*/
        // update biases and weights
        update_weights(alpha);
/*
        for (int b = 0; b < total_bb_size_; ++b) {
//            if (regres_)
                bb_[b] -= alpha * bb_deriv_[b];
//            else {
//                T s = bb_deriv_[b] >= 0. ? (bb_deriv_[b] == 0 ? 0. : 1.) : -1.;
//                bb_[b] -= alpha * s;
//            }
        }

        for (int w = 0; w < total_ww_size_; ++w) {
//            if (regres_)
                ww_[w] -= alpha * ww_deriv_[w];
//            else {
//                T s = ww_deriv_[w] >= 0. ? (ww_deriv_[w] == 0. ? 0. : 1.) : -1.;
//                ww_[w] -= alpha * s;
//            }
        }
*/
        return cost / rows + reg;
    }

    double regularize(double lambda, int rows) {
        double reg = 0.;

        for (int w = 0; w < total_ww_size_; ++w) {
            reg += lambda * ww_[w] * ww_[w] / (2. * rows);
            ww_deriv_[w] += lambda * ww_[w] / rows;
        }

        return reg;
    }


    void update_weights(T alpha) {
        for (int b = 0; b < total_bb_size_; ++b) {
//            if (regres_)
                bb_[b] -= alpha * bb_deriv_[b];
//            else {
//                T s = bb_deriv_[b] >= 0. ? (bb_deriv_[b] == 0 ? 0. : 1.) : -1.;
//                bb_[b] -= alpha * s;
//            }
        }

        for (int w = 0; w < total_ww_size_; ++w) {
//            if (regres_)
                ww_[w] -= alpha * ww_deriv_[w];
//            else {
//                T s = ww_deriv_[w] >= 0. ? (ww_deriv_[w] == 0. ? 0. : 1.) : -1.;
//                ww_[w] -= alpha * s;
//            }
        }
    }

    void predict(const T* X, T* predictions, int rows) {
        int x_columns_num = sizes_[0];
        int y_columns_num = sizes_[ sizes_.size() - 1 ];

        for (int r = 0; r < rows; ++r) {
            forward(&X[r * x_columns_num]);
            get_output(&predictions[r * y_columns_num]);
        }
    }


    void get_output(T* y) {
        int layers_num = sizes_.size();
        int aa_idx = total_aa_size_ - sizes_[layers_num - 1];
        for (int a = 0; a < sizes_[layers_num - 1]; ++a) {
            y[a] = aa_[aa_idx + a];
        }
    }


    // Helper method to calculate cost of current sample
    T cost(const T* y) {
        int layers_num = sizes_.size();
        int aa_idx = total_aa_size_ - sizes_[layers_num - 1];
        T cost = 0;

        for (int a = 0; a < sizes_[layers_num - 1]; ++a) {
            //cost += (aa_[aa_idx + a] - y[a]) * (aa_[aa_idx + a] - y[a]);

            cost += -1. * (y[a] * ::log(aa_[aa_idx + a]) + (1. - y[a]) * ::log(1. - aa_[aa_idx + a]));
        }

        return cost;  // / 2.;
    }

    // Helper method to debug backward propagation logic
    void calc_deriv(const T* x, const T* y, memory::ptr_vec<T>& bd, memory::ptr_vec<T>& wd) {
        T epsilon = 0.0000001;

        // prepare vectors
        bd.reset(new T[total_bb_size_]);
        wd.reset(new T[total_ww_size_]);

        // biases
        for (int b = 0; b < total_bb_size_; ++b) {
            // save current bb value before changing
            T tmp_b = bb_[b];

            // 1st pass
            bb_[b] -= epsilon;
            forward(x);
            T cost_before = cost(y);
            bb_[b] = tmp_b;

            // 2nd pass
            bb_[b] += epsilon;
            forward(x);
            T cost_after = cost(y);

            // calc derivative
            T deriv = (cost_after - cost_before) / (2. * epsilon);
            bd[b] = deriv;

            // restore bb val
            bb_[b] = tmp_b;
        }

        // weights
        for (int w = 0; w < total_ww_size_; ++w) {
            // save current ww value before changing
            T tmp_w = ww_[w];

            // 1st pass
            ww_[w] -= epsilon;
            forward(x);
            T cost_before = cost(y);
            ww_[w] = tmp_w;

            // 2nd pass
            ww_[w] += epsilon;
            forward(x);
            T cost_after = cost(y);

            // calc derivative
            T deriv = (cost_after - cost_before) / (2. * epsilon);
            wd[w] = deriv;

            // restore ww val
            ww_[w] = tmp_w;
        }

    }

    const T* get_bb_deriv() const { return bb_deriv_.get(); }
    const T* get_ww_deriv() const { return ww_deriv_.get(); }

};



template<class T>
class RedANN : public IDecision<T> {

    std::vector<ann_leaner<T>* > anns_;
    std::vector<T> ww_;
    std::vector<T> ww_deriv_;
    std::vector<T> ww_mem_;
    std::vector<T> deltas_;

    double b_;

    std::vector<int> sizes_;


public:

    RedANN(const std::vector<int>& sizes, int ann_num) :
        b_(0.),
        sizes_(sizes)
    {
        int regres = 1;

        for (int i = 0; i < ann_num; ++i) {
            ma::random::seed(time(NULL) + i * i);

            ann_leaner<T>* ann = new ann_leaner<T>(sizes, regres, (IDecision<T>*)this, i);
            anns_.push_back(ann);

            T tmp;
            random::randn<T>(&tmp, 1, 0., .4);
            ww_.push_back(tmp);
            //ww_.push_back(1. / ann_num);
            ww_mem_.push_back(0);
            ww_deriv_.push_back(0);

            b_ = 0.;

            deltas_.push_back((T)0.);
        }
    }

    ~RedANN() {
        for (int a = 0; a < anns_.size(); ++a) {
            delete anns_[a];
        }
    }

    T delta(int dec_id) {
        return deltas_[dec_id];
    }

    double feed(const T* x, int cols, int rows, const T* y, double alpha, double lambda) {

        double cost = 0.;
        T tmp;
        double b_deriv = 0.;

        double res[anns_.size()];

        for (int i = 0; i < rows; ++i) {
            double sum = 0.;
            for (int a = 0; a < anns_.size(); ++a) {
                anns_[a]->forward(&x[i*cols]);
                anns_[a]->get_output(&tmp);

                sum += tmp * ww_[a] + b_;

                res[a] = tmp;

                //deltas_[a] = tmp - y[i];

                //
                ww_deriv_[a] = 0.;
            }

            //std::sort(&res[0], &res[3]);
            //sum = res[anns_.size() / 2];
            double delta = sum - y[i];

            b_deriv += delta;

            for (int a = 0; a < anns_.size(); ++a) {
                deltas_[a] = delta * ww_[a];
                anns_[a]->backward(&x[i*cols], &y[i]);

                //anns_[a]->get_output(&tmp);
                ww_deriv_[a] += delta * res[a];
            }

            cost += delta * delta;
        }

        double reg = 0.;

        for (int a = 0; a < anns_.size(); ++a) {
            anns_[a]->average_deriv((T)rows);
            reg += anns_[a]->regularize(lambda, rows);
            anns_[a]->update_weights(alpha);

            ww_deriv_[a] /= rows;
            ww_[a] -= alpha * ww_deriv_[a];
        }

        b_deriv /= rows;
        b_ -= alpha * b_deriv;

        reg /= anns_.size();

        return cost / rows + reg;
    }


    void predict(const T* x, T* predictions, int rows) {
        int cols = sizes_[0];
        T tmp;

        for (int i = 0; i < rows; ++i) {
            double sum = 0.;
            for (int a = 0; a < anns_.size(); ++a) {
                anns_[a]->forward(&x[i*cols]);
                anns_[a]->get_output(&tmp);
                sum += tmp * ww_[a];
            }

            predictions[i] = sum;
        }
    }




    void save_weights() {
        linalg::copy(&ww_mem_[0], &ww_[0], anns_.size());
        for (int a = 0; a < anns_.size(); ++a) {
            anns_[a]->save_weights();
        }
    }

    void restore_weights() {
        linalg::copy(&ww_[0], &ww_mem_[0], anns_.size());
        for (int a = 0; a < anns_.size(); ++a) {
            anns_[a]->restore_weights();
        }
    }

};




}




// interface to Python
extern "C" {

    void* ann_create(const int* layers, int size, int redundancy) {
        ma::random::seed();

        std::vector<int> sizes;

        for (int i = 0; i < size; ++i) {
            sizes.push_back(layers[i]);
        }

        ma::RedANN<double>* rann = new ma::RedANN<double>(sizes, redundancy);

        return rann;

    }

    void ann_free(void* rann) {
        delete static_cast< ma::RedANN<double>* >(rann);
    }

    void ann_fit(void* rann, const double* X, const double* Y, int cols, int rows, double* alpha, double lambda, double* cost) {
        *cost = static_cast< ma::RedANN<double>* >(rann)->feed(X, cols, rows, Y, *alpha, lambda);
    }

    void ann_predict(void* rann, const double* X, double* predictions, int rows) {
        static_cast< ma::RedANN<double>* >(rann)->predict(X, predictions, rows);
    }

    void ann_save(void* rann) {
        static_cast< ma::RedANN<double>* >(rann)->save_weights();
    }

    void ann_restore(void* rann) {
        static_cast< ma::RedANN<double>* >(rann)->restore_weights();
    }

}


















#endif



