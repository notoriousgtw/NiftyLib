#pragma once

#include <any>
#include <functional>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>

namespace Nifty::SVG
{

using Callback = std::function<void(const T&)>;

template<typename T>
class Observable;

template<typename T>
class Observer;

template<typename T>
concept IsObserver = std::is_base_of_v<Observer<std::any>, T>;

template<typename V>
concept IsObservable = std::is_base_of_v<Observable<std::any>, V>;

template<IsObserver T, IsObservable V>
class ObserverMatrix;

template<typename T>
class Observer
{
  public:
	Observer(Observable<T>* observable, Callback callback = nullptr): observable(observable), callback(callback) {};
	~Observer<T>()
	{
		if (observable) observable->RemoveObserver
	}
	Observable<T>* observable;
	Callback	   callback;
};

template<typename T>
class Observable
{
  public:
	Observable(const T& initial): data(initial) {};

	void Set(const T& new_value)
	{
		data = new_value;
		for (auto& observer : observers)
			if (observer.callback) observer.callback(data);
	}

	const T&	Get() const { return data; };
	Observable& operator=(const T& new_value)
	{
		data = new_value;
		for (auto& observer : observers)
			if (observer.callback) observer.callback(data);
	};
	void RemoveObserver(Observer<T>* observer) { this->observers.erase(observer); }

  private:
	T					   data;
	std::set<Observer<T>*> observers;
};

template<IsObserver T, IsObservable V>
class ObserverMatrix
{
  public:
	void AddObserver() { obvservers.insert(T(observable, callback)) };
	void RemoveObserver(T* observer)
	{
		observer->observable->RemoveObserver(observer);
		observers.erase(observer);
	};
	void AddObservable(V* observable) { observables.insert(observable) };
	void RemoveObservable(V observable);

  private:
	std::set<T*> obvservers;
	std::set<V*> obvservables;
};

enum class Color
{
	none,
	red,
	green,
	blue,
	white,
	black
};

const std::map<Color, std::string> Colors = { { Color::none, "none" },	 { Color::red, "red" },
											  { Color::green, "green" }, { Color::blue, "blue" },
											  { Color::white, "white" }, { Color::black, "black" } };

class Element
{
  public:
	Image*		 image;
	virtual void Calc() = 0;

  private:
	std::string element_data;
	bool		editing = false;
	size_t		indent;
};

class StyledElement: Element
{
  public:
	virtual void Calc() = 0;

  private:
	Observable<Color> fill = Observable<Color>(
		Color::none,
		[this](const Color& color) { style = std::regex_replace(style, std::regex("(fill: )(.*)(?=;)"), "$1"); });
	Color		stroke		 = Color::black;
	double		stroke_width = 1;
	std::string style		 = R"SVG(
	style="
		fill: none;
		stroke: black;
		stroke-width: 1;
	"
)SVG";
};

class Path: StyledElement
{
  public:
	Path();
	void Calc() override;

  private:
	std::string open = "<path";
	std::string body;
	std::string path_data = R"SVG(
	d=\"
DATA
	"
)SVG";
};

class ClipPath
{
  public:
	ClipPath() { GenId(); };
	bool		operator<(const ClipPath& other) const { return id < other.id; };
	std::string Calc();

  private:
	std::string	 id;
	virtual void GenId() = 0;
};

}	 // namespace Nifty::SVG
