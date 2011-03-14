#include <ecto/ecto.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <cmath>
#include "FASTHarris.h"
template<typename T>
  std::ostream& operator<<(std::ostream& out, const std::vector<T>& t)
  {
    out << "[";
    BOOST_FOREACH(const T& x, t)
            out << x << " ";
    return out << "]";
  }
std::ostream& operator<<(std::ostream& out, const std::vector<cv::KeyPoint>& t)
{
  out << "[";
  BOOST_FOREACH(const cv::KeyPoint& x, t)
          out << boost::format("(%f,%f) ") % x.pt.x % x.pt.y;
  return out << "]";
}
struct Pyramid : ecto::module
{
  Pyramid() :
    levels_(3), magnification_(0), scale_factor_(1.42)
  {
    inout();
  }
  void inout()
  {
    for (int i = 0; i < levels_; i++)
    {
      setOut<cv::Mat> (str(boost::format("out_%04d") % i));
      setOut<float> (str(boost::format("scale_%04d") % i));
    }
    setIn<cv::Mat> ("in");
  }
  void Config(int levels, float scale_factor, int magnification)
  {
    levels_ = levels;
    scale_factor_ = scale_factor;
    magnification_ = magnification;
  }
  void process()
  {
    //std::vector<cv::Mat>& out = getOut<std::vector<cv::Mat> > ("out");
    // std::vector<float>& scales = getOut<std::vector<float> > ("scales");
    const cv::Mat& in = getIn<cv::Mat> ("in");
    //out.resize(levels_);
    //scales.resize(levels_);
    for (int i = 0; i < levels_; i++)
    {
      cv::Mat& out = getOut<cv::Mat> (str(boost::format("out_%04d") % i));
      float & scale = getOut<float> (str(boost::format("scale_%04d") % i));
      scale = 1.0f / float(std::pow(scale_factor_, i - magnification_));
      //use liner interp if magnifying, area based if decimating
      cv::resize(in, out, cv::Size(), scale, scale, i - magnification_ < 0 ? CV_INTER_LINEAR : CV_INTER_AREA);
    }
  }
  int levels_, magnification_;
  float scale_factor_;
};
struct FAST : ecto::module
{
  FAST() :
    thresh_(20)
  {
    inout();
  }
  void inout()
  {
    setOut<std::vector<cv::KeyPoint> > ("out", "Detected keypoints");
    setIn<cv::Mat> ("in" , "The image to detect FAST on.");
    setIn<cv::Mat> ("mask", "optional mask");
  }
  void Config(int thresh)
  {
    thresh_ = thresh;
  }
  void process()
  {
    const cv::Mat& in = getIn<cv::Mat> ("in");
    const cv::Mat& mask = getIn<cv::Mat> ("mask");
    std::vector<cv::KeyPoint>& kpts = getOut<std::vector<cv::KeyPoint> > ("out");
    cv::FastFeatureDetector fd(thresh_, true);
    fd.detect(in, kpts, mask);
  }

  int thresh_;
};

struct Harris : ecto::module
{
  Harris()
  {
    inout();
  }
  void inout()
  {
    setOut<std::vector<cv::KeyPoint> > ("out", "Detected keypoints, with Harris");
    setIn<cv::Mat> ("image" , "The image to calc harris response from.");
    setIn<std::vector<cv::KeyPoint> > ("kpts", "The keypoints to fill with Harris response.");
  }
  void Config()
  {
  }
  void process()
  {
    const cv::Mat& image = getIn<cv::Mat> ("image");
    const std::vector<cv::KeyPoint>& kpts_in = getIn<std::vector<cv::KeyPoint> > ("kpts");
    std::vector<cv::KeyPoint>& kpts = getOut<std::vector<cv::KeyPoint> > ("out");
    HarrisResponse h(image);
    kpts = kpts_in;
    h(kpts);
  }

  int thresh_;
};
ECTO_MODULE(orb)
{
  ecto::wrap<Pyramid>("Pyramid");
  ecto::wrap<FAST>("FAST");
  ecto::wrap<Harris>("Harris");
}
