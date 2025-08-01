#pragma once

namespace Nifty::Observe
{

template<typename T>
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

}