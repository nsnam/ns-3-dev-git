#!/usr/bin/env python

"""
Companion TwoRaySpectrumPropagationLossModel calibration script

Copyright (c) 2022 SIGNET Lab, Department of Information Engineering,
University of Padova

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.
"""

import pandas as pd
from matplotlib import pyplot as plt
import seaborn as sns
import numpy as np
from pathlib import Path
from itertools import product
from tqdm import tqdm
import joblib
import contextlib
import argparse as argp


# Command line arguments
parser = argp.ArgumentParser(formatter_class=argp.ArgumentDefaultsHelpFormatter)
parser.add_argument("--num_search_grid_params", default=30,
                    help="Number of values for each parameter of the search grids")
parser.add_argument("--num_refinements", default=1,
                    help="Number of refinement local search runs to be carried out")
parser.add_argument("--ref_data_fname", default="two-ray-to-three-gpp-splm-calibration.csv",
                    help="Filename of the fit reference data, obtained from ns-3")
parser.add_argument("--fit_out_fname", default="two-ray-splm-fitted-params.txt",
                    help="Filename of the fit results")
parser.add_argument("--c_plus_plus_out_fname", default="two-ray-cplusplus-fitted-params.txt",
                    help="Filename of the fit results, encoded as a C++ data structure to be imported in ns-3")
parser.add_argument("--figs_folder", default="FiguresTwoRayThreeGppChCalibration/",
                    help="Output folder for the fit results figures")
parser.add_argument("--epsilon", default=1e-7,
                    help="Tolerance value for the preliminary tests")
parser.add_argument("--preliminary_fit_test", default=True,
                    help="Whether to run preliminary tests which check the correctness of the script functions")
parser.add_argument("--fit_ftr_to_threegpp", default=True,
                    help="Whether to run the calibration with respect to the 3GPP reference channel gains")
parser.add_argument("--output_ns3_table", default=True,
                    help="Whether to output the code for importing the calibration results in ns-3")
parser.add_argument("--plot_fit_results", default=False,
                    help="Whether to plot a comparison of the reference data ECDFs vs the fitted FTR distributions")

args = parser.parse_args()
# Number of values for each parameter of the search grids
num_search_grid_params = int(args.num_search_grid_params)
# Number of refinement local search runs to be carried out
num_refinements = int(args.num_refinements)
# Filename of the fit reference data, obtained from ns-3
ref_data_fname = args.ref_data_fname
# Filename of the fit results
fit_out_fname = args.fit_out_fname
# Filename of the fit results, encoded as a C++ data structure to be imported in ns-3
c_plus_plus_out_fname = args.c_plus_plus_out_fname
# Output folder for the fit results figures
figs_folder = args.figs_folder
# Tolerance value for the preliminary tests
epsilon = float(args.epsilon)
# Whether to run preliminary tests which check the correctness of the script functions
preliminary_fit_test = bool(args.preliminary_fit_test)
# Whether to run the calibration with respect to the 3GPP reference channel gains
fit_ftr_to_threegpp = bool(args.fit_ftr_to_threegpp)
# Whether to output the code for importing the calibration results in ns-3
output_ns3_table = bool(args.output_ns3_table)
# Whether to plot a comparison of the reference data ECDFs vs the fitted FTR distributions
plot_fit_results = bool(args.plot_fit_results)


@contextlib.contextmanager
def tqdm_joblib(tqdm_object):
    """
      Context manager to patch joblib to report into tqdm progress bar given as argument.
      Taken from: https://stackoverflow.com/questions/24983493/tracking-progress-of-joblib-parallel-execution

    """
    class TqdmBatchCompletionCallback(joblib.parallel.BatchCompletionCallBack):
        def __call__(self, *args, **kwargs):
            tqdm_object.update(n=self.batch_size)
            return super().__call__(*args, **kwargs)

    old_batch_callback = joblib.parallel.BatchCompletionCallBack
    joblib.parallel.BatchCompletionCallBack = TqdmBatchCompletionCallback
    try:
        yield tqdm_object
    finally:
        joblib.parallel.BatchCompletionCallBack = old_batch_callback
        tqdm_object.close()


## FtrParams class
class FtrParams:
    ## @var m
    #  Parameter m for the Gamma variable. Used both as the shape and rate parameters.
    ## @var sigma
    #  Parameter sigma. Used as the variance of the amplitudes of the normal diffuse components.
    ## @var k
    #  Parameter K. Expresses ratio between dominant specular components and diffuse components.
    ## @var delta
    #  Parameter delta [0, 1]. Expresses how similar the amplitudes of the two dominant specular components are.

    def __init__(self, m: float, sigma: float, k: float, delta: float):
        '''! The initializer.
        @param self: the object pointer
        @param m: Parameter m for the Gamma variable. Used both as the shape and rate parameters.
        @param sigma: Parameter sigma. Used as the variance of the amplitudes of the normal diffuse components.
        @param k: Parameter K. Expresses ratio between dominant specular components and diffuse components.
        @param delta: Parameter delta [0, 1]. Expresses how similar the amplitudes of the two dominant specular components are.
        '''

        self.m = m
        self.sigma = sigma
        self.k = k
        self.delta = delta

    def __init__(self):
        '''! The initializer with default values.
        @param self: the object pointer
        '''

        self.m = 1
        self.sigma = 1.0
        self.k = 0.0
        self.delta = 0.0

    def __str__(self):
        '''! The initializer with default values.
        @param self: the object pointer
        @returns A string reporting the value of each of the FTR fading model parameters
        '''

        return f'm: {self.m}, sigma: {self.sigma}, k: {self.k}, delta: {self.delta}'


def get_ftr_ecdf(params: FtrParams, n_samples: int, db=False):
    '''!  Returns the ECDF for the FTR fading model, for a given parameter grid.
        @param params: The FTR parameters grid.
        @param n_samples: The number of samples of the output ECDF
        @param db: Whether to return the ECDF with the gain expressed in dB
        @returns The ECDF for the FTR fading model
    '''

    assert (params.delta >= 0 and params.delta <= 1.0)

    # Compute the specular components amplitudes from the FTR parameters
    cmn_sqrt_term = np.sqrt(1 - params.delta**2)
    v1 = np.sqrt(params.sigma) * np.sqrt(params.k * (1 - cmn_sqrt_term))
    v2 = np.sqrt(params.sigma) * np.sqrt(params.k * (1 + cmn_sqrt_term))

    assert (abs((v1**2 + v2**2)/(2*params.sigma) - params.k) < 1e-5)
    if params.k > 0:
        assert (abs((2*v1*v2)/(v1**2 + v2**2) - params.delta) < 1e-4)
    else:
        assert (v1 == v2 == params.k)

    sqrt_gamma = np.sqrt(np.random.gamma(
        shape=params.m, scale=1/params.m, size=n_samples))

    # Sample the random phases of the specular components, which are uniformly distributed in [0, 2*PI]
    phi1 = np.random.uniform(low=0, high=1.0, size=n_samples)
    phi2 = np.random.uniform(low=0, high=1.0, size=n_samples)

    # Sample the normal-distributed real and imaginary parts of the diffuse components
    x = np.random.normal(scale=np.sqrt(params.sigma), size=n_samples)
    y = np.random.normal(scale=np.sqrt(params.sigma), size=n_samples)

    # Compute the channel response by combining the above terms
    compl_phi1 = np.vectorize(complex)(np.cos(phi1), np.sin(phi1))
    compl_phi2 = np.vectorize(complex)(np.cos(phi2), np.sin(phi2))
    compl_xy = np.vectorize(complex)(x, y)
    h = np.multiply(sqrt_gamma, compl_phi1) * v1 + \
        np.multiply(sqrt_gamma, compl_phi2) * v2 + compl_xy

    # Compute the squared norms
    power = np.square(np.absolute(h))

    if db:
        power = 10*np.log10(power)

    return np.sort(power)


def compute_ftr_mean(params: FtrParams):
    '''!  Computes the mean of the FTR fading model, given a specific set of parameters.
        @param params: The FTR fading model parameters.
    '''

    cmn_sqrt_term = np.sqrt(1 - params.delta**2)
    v1 = np.sqrt(params.sigma) * np.sqrt(params.k * (1 - cmn_sqrt_term))
    v2 = np.sqrt(params.sigma) * np.sqrt(params.k * (1 + cmn_sqrt_term))

    mean = v1**2 + v2**2 + 2*params.sigma

    return mean


def compute_ftr_th_mean(params: FtrParams):
    '''!  Computes the mean of the FTR fading model using the formula reported in the corresponding paper,
          given a specific set of parameters.
        @param params: The FTR fading model parameters.
    '''

    return 2 * params.sigma * (1 + params.k)


def compute_anderson_darling_measure(ref_ecdf: list, target_ecdf: list) -> float:
    '''!  Computes the Anderson-Darling measure for the specified reference and targets distributions.
          In particular, the Anderson-Darling measure is defined as:
          A^2 = -N -S, where S = \sum_{i=1}^N \frac{2i - 1}{N} \left[ ln F(Y_i) + ln F(Y_{N + 1 - i}) \right].

          See https://www.itl.nist.gov/div898/handbook/eda/section3/eda35e.htm for further details.

        @param ref_ecdf: The reference ECDF.
        @param target_ecdf: The target ECDF we wish to match the reference distribution to.
        @returns The Anderson-Darling measure for the specified reference and targets distributions.
    '''

    assert (len(ref_ecdf) == len(target_ecdf))

    n = len(ref_ecdf)
    mult_factors = np.linspace(start=1, stop=n, num=n)*2 + 1
    ecdf_values = compute_ecdf_value(ref_ecdf, target_ecdf)

    # First and last elements of the ECDF may lead to NaNs
    with np.errstate(divide='ignore'):
        log_a_plus_b = np.log(ecdf_values) + np.log(1 - np.flip(ecdf_values))

    valid_idxs = np.isfinite(log_a_plus_b)
    A_sq = - np.dot(mult_factors[valid_idxs], log_a_plus_b[valid_idxs])

    return A_sq


def compute_ecdf_value(ecdf: list, data_points: float) -> np.ndarray:
    '''!  Given an ECDF and data points belonging to its domain, returns their associated EDCF value.
        @param ecdf: The ECDF, represented as a sorted list of samples.
        @param data_point: A list of data points belonging to the same domain as the samples.
        @returns The ECDF value of the domain points of the specified ECDF
    '''

    ecdf_values = []
    for point in data_points:
        idx = np.searchsorted(ecdf, point) / len(ecdf)
        ecdf_values.append(idx)

    return np.asarray(ecdf_values)


def get_sigma_from_k(k: float) -> float:
    '''!  Computes the value for the FTR parameter sigma, given k, yielding a unit-mean fading process.
        @param k: The K parameter of the FTR fading model, which represents the ratio of the average power
                  of the dominant components to the power of the remaining diffuse multipath.
        @returns The value for the FTR parameter sigma, given k, yielding a unit-mean fading process.
    '''

    return 1 / (2 + 2 * k)


def fit_ftr_to_reference(ref_data: pd.DataFrame, ref_params_combo: tuple,  num_params: int, num_refinements: int) -> str:
    '''!  Estimate the FTR parameters yielding the closest ECDF to the reference one.

          Uses a global search to estimate the FTR parameters yielding the best fit to the reference ECDF.
          Then, the search is refined by repeating the procedure in the neighborhood of the parameters
          identified with the global search. Such a neighborhood is determined as the interval whose center
          is the previous iteration best value, and the lower and upper bounds are the first lower and upper
          values which were previously considered, respectively.

        @param ref_data: The reference data, represented as a DataFrame of samples.
        @param ref_params_combo: The specific combination of simulation parameters corresponding
                                 to the reference ECDF
        @param num_params: The number of values of each parameter in the global and local search grids.
        @param num_refinements: The number of local refinement search to be carried out after the global search.

        @returns An estimate of the FTR parameters yielding the closest ECDF to the reference one.
    '''

    # Retrieve the reference ECDF
    ref_ecdf = ref_data.query(
        'scen == @ref_params_combo[0] and cond == @ref_params_combo[1] and fc == @ref_params_combo[2]')

    # Perform the fit
    n_samples = len(ref_ecdf)
    best_params = FtrParams()
    best_ad = np.inf

    # The m and K parameters can range in ]0, +inf[
    m_and_k_ub = 4
    m_and_k_lb = 0.001
    m_and_k_step = (m_and_k_ub - m_and_k_lb) / n_samples

    # The delta parameter can range in [0, 1]
    delta_step = 1/n_samples

    # Define the coarse grid
    coarse_search_grid = {
        # m must be in [0, +inf]
        'm': np.power(np.ones(num_params)*10, np.linspace(start=m_and_k_lb, stop=m_and_k_ub, endpoint=True, num=num_params)),
        # k must be in [0, +inf]
        'k': np.power(np.ones(num_params)*10, np.linspace(start=m_and_k_lb, stop=m_and_k_ub, endpoint=True, num=num_params)),
        # delta must be in [0, 1]
        'delta': np.linspace(start=0.0, stop=1.0, endpoint=True, num=num_params)
        # sigma determined from k, due to the unit-mean constraint
    }

    for element in product(*coarse_search_grid.values()):

        # Create FTR params object
        params = FtrParams()
        params.m = element[0]
        params.k = element[1]
        params.delta = element[2]
        params.sigma = get_sigma_from_k(params.k)

        # Retrieve the corresponding FTR ECDF
        ftr_ecdf = get_ftr_ecdf(params, n_samples, db=True)
        ad_meas = compute_anderson_darling_measure(ref_ecdf, ftr_ecdf)

        if (ad_meas < best_ad):
            best_params = params
            best_ad = ad_meas

    for _ in range(num_refinements):
        # Refine search in the neighborhood of the previously identified params
        finer_search_grid = {
            'm': np.power(np.ones(num_params)*10,
                          np.linspace(start=max(0, np.log10(best_params.m) - m_and_k_step),
                                      stop=np.log10(best_params.m) +
                                      m_and_k_step,
                                      endpoint=True, num=num_params)),
            'k': np.power(np.ones(num_params)*10,
                          np.linspace(start=max(0, np.log10(best_params.k) - m_and_k_step),
                                      stop=np.log10(best_params.k) +
                                      m_and_k_step,
                                      endpoint=True, num=num_params)),
            'delta': np.linspace(start=max(0, best_params.delta - delta_step),
                                 stop=min(1, best_params.delta + delta_step),
                                 endpoint=True, num=num_params)
            # sigma determined from k, due to the unit-mean constraint
        }

        m_and_k_step = (np.log10(best_params.m) + m_and_k_step -
                        max(0, np.log10(best_params.m) - m_and_k_step)) / n_samples
        delta_step = (min(1, best_params.delta + 1/num_params) -
                      max(0, best_params.delta - 1/num_params)) / n_samples

        for element in product(*finer_search_grid.values()):

            # Create FTR params object
            params = FtrParams()
            params.m = element[0]
            params.k = element[1]
            params.delta = element[2]
            params.sigma = get_sigma_from_k(params.k)

            # Retrieve the corresponding FTR ECDF
            ftr_ecdf = get_ftr_ecdf(params, n_samples, db=True)
            ad_meas = compute_anderson_darling_measure(ref_ecdf, ftr_ecdf)

            if (ad_meas < best_ad):
                best_params = params
                best_ad = ad_meas

    out_str = f"{ref_params_combo[0]}\t{ref_params_combo[1]}\t{ref_params_combo[2]}" + \
        f" \t{best_params.sigma}\t{best_params.k}\t{best_params.delta}\t{best_params.m}\n"

    return out_str


def append_ftr_params_to_cpp_string(text: str, params: FtrParams) -> str:
    text += f'TwoRaySpectrumPropagationLossModel::FtrParams({np.format_float_scientific(params.m)}, {np.format_float_scientific(params.sigma)}, \
                                                            {np.format_float_scientific(params.k)}, {np.format_float_scientific(params.delta)})'

    return text


def print_cplusplus_map_from_fit_results(fit: pd.DataFrame, out_fname: str):
    """
    Prints to a file the results of the FTR fit, as C++ code.

    Args:
      fit (pd.DataFrame): A Pandas Dataframe holding the results of the FTR fit.
      out_fname (str): The name of the file to print the C++ code to.
    """

    out_str = '{'

    for scen in set(fit['scen']):
        out_str += f'{{\"{scen}\",\n{{'

        for cond in set(fit['cond']):
            out_str += f'{{ChannelCondition::LosConditionValue::{cond}, \n'

            # Print vector of carrier frequencies
            freqs = np.sort(list(set(fit['fc'])))
            out_str += "{{"
            for fc in freqs:
                out_str += f'{float(fc)}, '
            out_str = out_str[0:-2]
            out_str += '},\n{'

            # Load corresponding fit results
            for fc in freqs:
                fit_line = fit.query(
                    'scen == @scen and cond == @cond and fc == @fc')
                assert(fit_line.reset_index().shape[0] == 1)

                params = FtrParams()
                params.m = fit_line.iloc[0]['m']
                params.k = fit_line.iloc[0]['k']
                params.delta = fit_line.iloc[0]['delta']
                params.sigma = fit_line.iloc[0]['sigma']

                # Print vector of corresponding FTR parameters
                out_str = append_ftr_params_to_cpp_string(out_str, params)
                out_str += ', '

            out_str = out_str[0:-2]
            out_str += '}'
            out_str += '}},\n'

        out_str = out_str[0:-2]
        out_str += '}},\n'

    out_str = out_str[0:-2]
    out_str += '}\n'

    f = open(out_fname, "w")
    f.write(out_str)
    f.close()


if __name__ == '__main__':

    #########################
    ## Data pre-processing ##
    #########################

    # Load reference data obtained from the ns-3 TR 38.901 implementation
    df = pd.read_csv(ref_data_fname, sep='\t')
    # Linear gain --> gain in dB
    df['gain'] = 10*np.log10(df['gain'])

    # Retrieve the possible parameters configurations
    scenarios = set(df['scen'])
    is_los = set(df['cond'])
    frequencies = np.sort(list(set(df['fc'])))

    ####################################################################################################
    ## Fit Fluctuating Two Ray model to the 3GPP TR 38.901 using the Anderson-Darling goodness-of-fit ##
    ####################################################################################################

    if preliminary_fit_test:

        params = FtrParams()
        get_ftr_ecdf(params, 100)

        # Make sure the mean is indeed independent from the delta parameter
        mean_list = []
        params = FtrParams()
        for delta in np.linspace(start=0, stop=1, num=100):
            params.delta = delta
            mean_list.append(compute_ftr_mean(params))

        avg_mean = np.mean(mean_list)
        assert np.all(np.abs(mean_list - avg_mean) < epsilon)

        # Make sure that we are indeed generating parameters yielding unit-mean
        mean_list.clear()
        mean_th_list = []
        params = FtrParams()
        for k in np.linspace(start=1, stop=500, num=50):
            sigma = get_sigma_from_k(k)
            params.sigma = sigma
            params.k = k
            mean_list.append(compute_ftr_mean(params))
            mean_th_list.append(compute_ftr_th_mean(params))

        assert np.all(np.abs(mean_list - np.float64(1.0)) < epsilon)
        assert np.all(np.abs(mean_th_list - np.float64(1.0)) < epsilon)

    if fit_ftr_to_threegpp:

        # Parallel search for the different simulation parameters combination
        with tqdm_joblib(tqdm(desc="Fitting FTR to the 3GPP fading model", total=(len(scenarios) * len(is_los) * len(frequencies)))) as progress_bar:
            res = joblib.Parallel(n_jobs=10)(
                joblib.delayed(fit_ftr_to_reference)(df, params_comb, num_search_grid_params, num_refinements) for params_comb in product(scenarios, is_los, frequencies))

        f = open(fit_out_fname, "w")
        f.write("scen\tcond\tfc\tsigma\tk\tdelta\tm\n")
        for line in res:
            f.write(line)

        f.close()

    if output_ns3_table:

        # Load the fit results
        fit = pd.read_csv(fit_out_fname, delimiter='\t')

        # Output the C++ data structure
        print_cplusplus_map_from_fit_results(fit, c_plus_plus_out_fname)

    if plot_fit_results:

        # Set Seaborn defaults and setup output folder
        sns.set(rc={'figure.figsize': (7, 5)})
        sns.set_theme()
        sns.set_style('darkgrid')

        fit = pd.read_csv(fit_out_fname, delimiter='\t')
        # Create folder if it does not exist
        Path(figs_folder).mkdir(parents=True, exist_ok=True)
        ad_measures = []

        for params_comb in product(scenarios, is_los, frequencies):

            data_query = 'scen == @params_comb[0] and cond == @params_comb[1] and fc == @params_comb[2]'

            # Load corresponding reference data
            ref_data = df.query(data_query)

            # Create FTR params object
            fit_line = fit.query(data_query)
            assert(fit_line.reset_index().shape[0] == 1)
            params = FtrParams()
            params.m = fit_line.iloc[0]['m']
            params.k = fit_line.iloc[0]['k']
            params.delta = fit_line.iloc[0]['delta']
            params.sigma = fit_line.iloc[0]['sigma']

            # Retrieve the corresponding FTR ECDF
            ftr_ecdf = get_ftr_ecdf(params, len(ref_data), db=True)

            # Compute the AD measure
            ad_meas = compute_anderson_darling_measure(
                np.sort(ref_data['gain']), ftr_ecdf)
            ad_measures.append(np.sqrt(ad_meas))

            sns.ecdfplot(data=ref_data, x='gain',
                         label='38.901 reference model')
            sns.ecdfplot(
                ftr_ecdf, label=f'Fitted FTR, sqrt(AD)={round(np.sqrt(ad_meas), 2)}')
            plt.xlabel(
                'End-to-end channel gain due to small scale fading [dB]')
            plt.legend()
            plt.savefig(
                f'{figs_folder}{params_comb[0]}_{params_comb[1]}_{params_comb[2]/1e9}GHz_fit.png', dpi=500, bbox_inches='tight')
            plt.clf()

        # Plot ECDF of the scaled and normalized AD measures
        sns.ecdfplot(ad_measures, label='AD measures')
        plt.xlabel('Anderson-Darling goodness-of-fit')
        plt.legend()
        plt.savefig(f'{figs_folder}AD_measures.png',
                    dpi=500, bbox_inches='tight')
        plt.clf()
