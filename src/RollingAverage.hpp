#include <algorithm>
#include <deque>
#include <numeric>

class RollingAverage
{
  std::deque<double> values;
  size_t maxSize;

public:
  RollingAverage(size_t size) : maxSize(size)
  {
  }

  void add(double value)
  {
    if (values.size() == maxSize)
    {
      values.pop_front(); // Remove oldest value
    }
    values.push_back(value); // Add new value
  }

  double average() const
  {
    if (values.empty())
    {
      return 0.0; // Return 0 if the deque is empty to avoid division by zero
    }

    std::deque<double> sortedValues = values;
    std::sort(sortedValues.begin(), sortedValues.end());

    size_t quarter = sortedValues.size() / 4;
    double sum = std::accumulate(sortedValues.begin() + quarter,
                                 sortedValues.end() - quarter, 0.0);
    return sum / (sortedValues.size() - 2 * quarter);
  }
};