#ifndef SPRITE_H
#define SPRITE_H

#include <QObject>
#include "texture.h"

class Sprite
{
  public:
  explicit Sprite();
  explicit Sprite(const Sprite &S);

  Sprite& operator=(const Sprite& S);
  void set_image(QString type, QImage i);
  bool get_image(QString type, QImage *dst);

  void set_texture(QString type, Texture t);
  Texture get_texture(QString type);

  private:
  Texture diffuse;
  Texture normal;
  Texture specular;
  Texture parallax;
  Texture occlussion;
  Texture heightmap;
  Texture distance;
  Texture neighbours;
};

#endif // SPRITE_H