// file      : common/enum/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <odb/core.hxx>

enum color {red, green, blue};

#pragma db object
struct object
{
  #pragma db id auto
  unsigned long id_;

  color color_;
  enum taste {bitter, sweet, sour};
  taste taste_;

  enum position {left = -1, center = 0, right = 1};
  position position_;


  enum class gender {male, female};
  enum class scale: unsigned char {one = 1, ten = 10, hundred = 100};
  enum class yesno: bool {no, yes};

  gender gender_;
  scale scale_;
  yesno yesno_;
};

inline bool
operator == (const object& x, const object& y)
{
  return
    x.id_ == y.id_
    && x.color_ == y.color_
    && x.taste_ == y.taste_
    && x.position_ == y.position_
    && x.gender_ == y.gender_
    && x.scale_ == y.scale_
    && x.yesno_ == y.yesno_;
}

#endif // TEST_HXX
