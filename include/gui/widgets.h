#pragma once

namespace nft::gui
{
class Widget
{
  public:
	Widget()  = default;
	~Widget() = default;
	void SetPosition(int x, int y)
	{
		pos_x = x;
		pos_y = y;
	}
	void SetSize(int width, int height)
	{
		this->width	 = width;
		this->height = height;
	}
	int GetX() const { return pos_x; }
	int GetY() const { return pos_y; }
	int GetWidth() const { return width; }
	int GetHeight() const { return height; }

  private:
	glm::vec2 position {};
	glm::vec2 size { 100.0f, 30.0f };
};
}	 // namespace nft::gui