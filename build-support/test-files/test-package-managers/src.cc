#include <armadillo>

int
main(int argc, char** argv)
{
    arma::arma_rng::set_seed_random();
    arma::Mat<double> mat = arma::randu(4, 4);
    std::cout << "mat:\n" << mat << "\n";
    return 0;
}
