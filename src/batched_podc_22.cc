/* Code for producing the experiments for the b-Batched setting in Section 12 of 
      "Balanced Allocations with the Choice of Noise"
      by Dimitrios Los and Thomas Sauerwald (PODC'22)
      [https://arxiv.org/abs/2302.04399]. */
#include <algorithm>
#include <iostream>
#include <map>
#include <random>

/* Runs the Two-Choice process in the b-Batched setting. This process was
   introduced in
     "Multiple-choice balanced allocation in (almost) parallel", 
         by Berenbrink, Czumaj, Englert, Friedetzky, and Nagel (2012)
         [https://arxiv.org/abs/1501.04822].
   
   It starts from an empty load vector and in each round:
     - Allocates b (potentially weighted) balls using the process provided,
       with the load information at the beginning of the batch.
   
   This class keeps track of the load-vector, the maximum load and gap.
   */
class BatchedTwoChoiceSetting {
public:

  /* Initializes b-Batched setting for the given number of bins 
     and batch size. */
  BatchedTwoChoiceSetting(size_t num_bins, size_t batch_size)    
    : load_vector_(num_bins, 0), buffer_vector_(num_bins, 0), uar_(0, num_bins - 1), batch_size_(batch_size), max_load_(0), total_balls_(0) {
    
  }
  
  /* Performs an allocation of a batch. */
  template<typename Generator>
  void nextRound(Generator& generator) {
    // Phase 1: Perform b allocations.
    size_t n = load_vector_.size();
    
    for (size_t i = 0; i < batch_size_; ++i) {
      size_t i1 = uar_(generator), i2 = uar_(generator);
      // Break ties randomly. 
      size_t idx = load_vector_[i1] <= load_vector_[i2] ? i1 : i2;
      ++buffer_vector_[idx];
    }
    total_balls_ += batch_size_;

    // Phase 2: Update and sort the load vector.
    for (int i = 0; i < n; ++i) {
      load_vector_[i] += buffer_vector_[i];
      buffer_vector_[i] = 0;
      max_load_ = std::max(max_load_, load_vector_[i]);
    }
    // std::sort(load_vector_.begin(), load_vector_.end(), std::greater<size_t>());
  }

  /* Returns the current maximum load. */
  double getMaxLoad() const {
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

  /* Current load vector of the process. */
  std::vector<size_t> load_vector_;

  /* Buffer vector for the balls allocated in the current batch. */
  std::vector<size_t> buffer_vector_;

  /* Sample a bin uniformly at random. */
  std::uniform_int<size_t> uar_;

  /* Batch size used in the setting. */
  const size_t batch_size_;

  /* Current maximum load in the load vector. */
  size_t max_load_;

  /* Total number of balls in the load vector. */
  size_t total_balls_;
};


void batched_experiments(int num_bins) {
  std::mt19937_64 generator;

  int runs = 100;
  std::vector<int> batch_sizes({ 5, 10, 50, 100, 500, 1'000, 5'000, 10'000, 50'000, 100'000, 500'000 });
  std::vector<std::pair<int, int>> one_choice_plot, two_choice_plot;
  
  std::cout << "=== Table 12.4 ===" << std::endl;
  for (auto batch_size : batch_sizes) {
    std::cout << "Batch-size (b) : " << batch_size << std::endl;
    int factor = batch_size >= num_bins ? 1'000 : 50;
    int num_rounds = factor * num_bins / batch_size;
    double one_choice_sum = 0.0, two_choice_sum = 0.0;
    std::map<int, int> one_choice_max_load_counts, two_choice_max_load_counts;
    for (int run = 0; run < runs; ++run) {
      BatchedTwoChoiceSetting batched_two_choice(num_bins, batch_size);
      for (int round = 0; round < num_rounds; ++round) {
        batched_two_choice.nextRound(generator);
        if (round == 0) {
          int current_gap = std::ceil(batched_two_choice.getGap());
          one_choice_max_load_counts[current_gap]++;
          one_choice_sum += current_gap;
        }
      }
      int current_gap = std::ceil(batched_two_choice.getGap());
      two_choice_sum += current_gap;
      two_choice_max_load_counts[current_gap]++;
    }
    one_choice_plot.push_back({ batch_size, one_choice_sum / runs });
    two_choice_plot.push_back({ batch_size, two_choice_sum / runs });
    std::cout << "Two-Choice:" << std::endl;
    for (const auto [load, load_count] : two_choice_max_load_counts) {
      std::cout << "\\textbf{" << load << "} : " << (load_count * 100 / runs) << "\\%" << std::endl;
    }
    std::cout << "One-Choice:" << std::endl;
    for (const auto [load, load_count] : one_choice_max_load_counts) {
      std::cout << "\\textbf{" << load << "} : " << (load_count * 100 / runs) << "\\%" << std::endl;
    }
    std::cout << std::endl;
  }
  std::cout << "=== Figure 12.2 ===" << std::endl;
  std::cout << "One-Choice:" << std::endl;
  for (const auto [x, y] : one_choice_plot) {
    std::cout << "(" << x << ", " << y << ")" << std::endl;
  }
  std::cout << "Two-Choice:" << std::endl;
  for (const auto [x, y] : two_choice_plot) {
    std::cout << "(" << x << ", " << y << ")" << std::endl;
  }
}

int main() {
  /* Runs experiments for Figure 12.2 and Table 12.4. */
  batched_experiments(10'000);
  
  return 0;
}
