/* Code for producing the experiments for the b-Batched setting in Section 12 of
      "Balanced Allocations with the Choice of Noise"
      by Dimitrios Los and Thomas Sauerwald (PODC'22)
      [https://arxiv.org/abs/2302.04399]. */
#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <random>

template<typename Generator>
using DeciderFn = std::function<size_t(const std::vector<size_t>&, size_t, size_t, Generator)>;

/* A process that makes two samples in each round and allocates 
   according to a decision function to one of the two. */
template<typename Generator>
class TwoSampleProcess {
public:

  /* Iniitializes the Two-Sample process. */
  TwoSampleProcess(
    size_t num_bins, 
    const DeciderFn<Generator> decider)
    : load_vector_(num_bins, 0), max_load_(0), total_balls_(0), uar_(0, num_bins - 1), decider_(decider) {

  }

  /* Performs an allocation of a batch. */
  void nextRound(Generator& generator) {
    size_t n = load_vector_.size();
    size_t i1 = uar_(generator);
    size_t i2 = uar_(generator);
    size_t idx = decider_(load_vector_, i1, i2, generator);
    ++load_vector_[idx];
    ++total_balls_;
    max_load_ = std::max(max_load_, load_vector_[idx]);
  }

  /* Returns the current maximum load. */
  size_t getMaxLoad() const {
    return max_load_;
  }

  /* Returns the current gap. */
  double getGap() const {
    return max_load_ - total_balls_ / double(load_vector_.size());
  }

  /* Returns the current load vector. */
  std::vector<size_t> getLoadVector() const {
    return load_vector_;
  }

private:

  /* Functon that decides in which of the two sampled bins to allocate to. */
  const DeciderFn<Generator> decider_;

  /* Current load vector of the process. */
  std::vector<size_t> load_vector_;

  /* Sample a bin uniformly at random. */
  std::uniform_int<size_t> uar_;

  /* Current maximum load in the load vector. */
  size_t max_load_;

  /* Total number of balls in the load vector. */
  size_t total_balls_;
};


template<typename Generator>
size_t two_choice(const std::vector<size_t>& load_vector, size_t i1, size_t i2, Generator& generator) {
  if (load_vector[i1] <= load_vector[i2]) return i1;
  return i2;
}

template<typename Generator>
DeciderFn<Generator> g_bounded(int g) {
  return [g](const std::vector<size_t>& load_vector, size_t i1, size_t i2, Generator& generator) {
    // Do normal Two-Choice.
    if (std::llabs(long long(load_vector[i1]) - long long(load_vector[i2])) > g) {
      if (load_vector[i1] <= load_vector[i2]) return i1;
      return i2;
    }
    // Reverse the allocation.
    if (load_vector[i1] <= load_vector[i2]) return i2;
    return i1;
  };
}

template<typename Generator>
DeciderFn<Generator> g_myopic(int g) {
  return [g](const std::vector<size_t>& load_vector, size_t i1, size_t i2, Generator& generator) {
    // If the difference is small, then randomise the allocation.
    if (std::llabs(long long(load_vector[i1]) - long long(load_vector[i2])) <= g) {
      std::bernoulli_distribution randomiser(0.5);
      return randomiser(generator) ? i1 : i2;
    }
    // Do normal Two-Choice.
    if (load_vector[i1] <= load_vector[i2]) return i1;
    return i2;
  };
}

template<typename Generator>
DeciderFn<Generator> sigma_noisy(int sigma) {
  return [sigma](const std::vector<size_t>& load_vector, size_t i1, size_t i2, Generator& generator) {
    std::normal_distribution<double> noise_distribution(0.0, sigma);
    long long load_estimate_1 = load_vector[i1] + noise_distribution(generator);
    long long load_estimate_2 = load_vector[i2] + noise_distribution(generator);
    if (load_estimate_1 <= load_estimate_2) return i1;
    return i2;
  };
}

template<typename Generator>
void normal_noise(
  int m_batches, 
  const std::vector<int>& param_values, 
  std::function<DeciderFn<Generator>(int)> decider_producer) {
  Generator generator;

  int runs = 100;
  std::vector<int> ns({ 10'000, 50'000, 100'000 });
  for (auto n : ns) {
    std::vector<std::pair<int, int>> coordinate_plot;
    std::cout << "n : " << n << std::endl << std::endl;
    for (const auto param : param_values) {
      std::cout << "Value : " << param << std::endl;
      TwoSampleProcess<Generator> two_choice_with_noice(n, decider_producer(param));
      int m = m_batches * n;
      double sum = 0.0;
      std::map<int, int> max_load_counts;
      for (int run = 0; run < runs; ++run) {
        for (int j = 0; j < m; ++j) {
          two_choice_with_noice.nextRound(generator);
        }
        int current_gap = two_choice_with_noice.getGap();
        sum += current_gap;
        max_load_counts[current_gap]++;
      }
      coordinate_plot.push_back({ param, sum / runs });
      for (const auto [load, load_count] : max_load_counts) {
        std::cout << "\\textbf{" << load << "} : " << (load_count * 100 / runs) << "\\%" << std::endl;
      }
      // std::cout << "g : " << g << " gives " <<  << std::endl;
    }
  }
}

std::vector<int> generate_range(int st, int en) {
  std::vector<int> ans;
  for (int i = st; i <= en; ++i) {
    ans.push_back(i);
  }
  return ans;
}

int main() {
  std::cout << "Sigma-noise: " << std::endl;
  normal_noise<std::mt19937_64>(1'000, generate_range(1, 20), sigma_noisy<std::mt19937_64>);
  std::cout << "g-Bounded: " << std::endl;
  normal_noise<std::mt19937_64>(1'000, generate_range(1, 20), g_bounded<std::mt19937_64>);
  std::cout << "g-Myopic: " << std::endl;
  normal_noise<std::mt19937_64>(1'000, generate_range(1, 20), g_myopic<std::mt19937_64>);
  return 0;
}
