#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <random>
#include <map>

const sf::Vector2<int> moves[] = {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}};

std::random_device dev;
std::mt19937 gen(dev());

class Object;
class Living;
class Posion;

class World;

class AgrInterface;
template<typename T>
class Agregator;

class Object {
public:
    sf::Vector2<int> position;
    sf::Color color;
 
    Object(sf::Vector2<int> position);

    virtual bool react(World* world, Living* acter);

    ~Object();
};

class Living : public Object {
public:
    int health;
    std::vector<int> code;
    int codePos;

    Living(sf::Vector2<int> position);

    void act(World* world);

    bool react(World* world, Living* acter);

    ~Living();
};

class Poison : public Object {
public:
    Poison(sf::Vector2<int> position);

    bool react(World* world, Living* acter);

    ~Poison();
};

class Wall : public Object {
public:
	Wall(sf::Vector2<int> position);

	bool react(World* world, Living* acter);

	~Wall();
};

class Healing : public Object {
public:
	Healing(sf::Vector2<int> position);

	bool react(World* world, Living* acter);

	~Healing();
};

class World {
private:
    std::map<std::string, AgrInterface*> agregators;
public:
	sf::Vector2<int> size;

    World(sf::Vector2<int> size);

    void remove(Object* object);

    void move(Living* living, sf::Vector2<int> newpos);

    bool checkPos(sf::Vector2<int> pos);

    void iterate();

    std::vector<std::pair<sf::Vector2<int>, sf::Color> > getDrawable();

    ~World();
};

class AgrInterface {
public:
	virtual void iterate(World* world) = 0;
	virtual bool remove(Object* object) = 0;
	virtual bool checkPos(sf::Vector2<int> pos) = 0;
	virtual Object* getOnPos(sf::Vector2<int> pos) = 0;
	virtual std::vector<Object*> getObjects() = 0;
};

template<typename objT>
class Agregator : public AgrInterface {
private:
	int minimum, maximum;
	std::vector<objT*> objects;
public:
	Agregator(int minimum, int maximum) :
	minimum(minimum),
	maximum(maximum) { }

	void iterate(World* world) {
		if(objects.size() <= minimum) {
			std::uniform_int_distribution<int> xdist(0, world->size.x - 1);
    		std::uniform_int_distribution<int> ydist(0, world->size.y - 1);

			for(int i = objects.size(); i < maximum; i++) {
		        sf::Vector2<int> newpos = {xdist(gen), ydist(gen)};
		        while(world->checkPos(newpos))
		            newpos = {xdist(gen), ydist(gen)};
		        objects.push_back(new objT(newpos));
			}
		}
	}

	Object* getOnPos(sf::Vector2<int> pos) {
		return *std::find_if(objects.begin(), objects.end(), 
	           [pos] (objT* o) { return o->position == pos; });
	}

	bool remove(Object* object) {
		auto it = std::find(objects.begin(), objects.end(), object);
	    if (it != objects.end()){
	        delete (*it);
	        objects.erase(it);
	        return true;
	    }
	    return false;
	}

	bool checkPos(sf::Vector2<int> pos) {
		return 	(std::find_if(objects.begin(), objects.end(), 
	            [pos] (objT* o) { return o->position == pos; }) 
				!= objects.end());
	}

	std::vector<Object*> getObjects() {
		std::vector<Object*> retval(objects.size());
		for(int i = 0; i < objects.size(); i++)
			retval[i] = (Object*)objects[i];
		return retval;
	}

	~Agregator() {
		for(auto o: objects)
			delete o;
	}
};

Object::Object(sf::Vector2<int> position) : 
    position(position) { }

bool Object::react(World* world, Living* acter) {
    return true;
}

Object::~Object() { }

Living::Living(sf::Vector2<int> position) : 
    Object(position),
    health(20),
    codePos(0) {
    color = sf::Color::Blue;

    std::uniform_int_distribution<int> sdist(20, 30);
    code.resize(sdist(gen));

    std::uniform_int_distribution<int> cdist(0, 7);

    for(int i = 0; i < code.size(); i++)
        code[i] = cdist(gen);
}

void Living::act(World* world) {
    health--;
    if(health <= 0) {
        world->remove(this);
        return;
    }

    codePos = (codePos + 1) % code.size();
    world->move(this, moves[code[codePos]]);
}

bool Living::react(World* world, Living* acter) {
    return false;
}

Living::~Living() { }



Poison::Poison(sf::Vector2<int> position) : 
    Object(position) {
    color = sf::Color::Red;
}

bool Poison::react(World* world, Living* acter) {
    acter->health -= 5;

    if(acter->health <= 0)
        world->remove(acter);
    world->remove(this);

    return true;
}

Poison::~Poison() { }

Wall::Wall(sf::Vector2<int> position) :
	Object(position) {
	color = sf::Color::White;
}

bool Wall::react(World* world, Living* acter) {
	return false;
}

Wall::~Wall() { }

Healing::Healing(sf::Vector2<int> position) :
	Object(position) {
	color = sf::Color::Green;
}

bool Healing::react(World* world, Living* acter) {
	acter->health += 5;

	world->remove(this);

	return true;
}

Healing::~Healing() { }


World::World(sf::Vector2<int> size) : size(size) {
    agregators["living"] = new Agregator<Living>(4, 8);
    agregators["posion"] = new Agregator<Poison>(5, 10);
    agregators["healing"] = new Agregator<Healing>(5, 10);
    agregators["wall"] = new Agregator<Wall>(10, 10);
}

bool World::checkPos(sf::Vector2<int> pos){
    for(auto agr: agregators) 
    	if(agr.second->checkPos(pos)) 
    		return true;
    return false;
}


void World::remove(Object* object){
    for(auto agr: agregators) 
    	if(agr.second->remove(object)) 
    		break;
}


void World::iterate(){
	for(auto agr: agregators)
		agr.second->iterate(this);

	for(Object* living: agregators["living"]->getObjects()) 
		((Living*)living)->act(this);
}

void World::move(Living* living, sf::Vector2<int> newpos){
    newpos += living->position;

    newpos.x = ((std::abs(newpos.x) / size.x + 1) * size.x + newpos.x) % size.x;
    newpos.y = ((std::abs(newpos.y) / size.y + 1) * size.y + newpos.y) % size.y;

    for(auto agr: agregators)
    	if(agr.second->checkPos(newpos)) {
    		if(!agr.second->getOnPos(newpos)->react(this, living))
    			return;
    		break;
    	}

    living->position = newpos;
}

std::vector<std::pair<sf::Vector2<int>, sf::Color> > World::getDrawable(){
	std::vector<std::pair<sf::Vector2<int>, sf::Color> > retval(0);
	for(auto agr: agregators)
		for(auto obj: agr.second->getObjects())
			retval.push_back(std::make_pair(obj->position, obj->color));
	return retval;
}

World::~World() {
    for(auto agr: agregators)
    	delete agr.second;
}

int main()
{
    sf::Vector2<int> size = {20, 20};
    const int scale = 25;
    const int delay = 1000;

    World w(size);

    sf::RenderWindow window(sf::VideoMode(size.x * scale, size.y * scale), "My window");
    sf::Clock clock;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear(sf::Color::Black);

        sf::RectangleShape square(sf::Vector2f(scale, scale));

        for(auto obj: w.getDrawable()){
            square.setPosition(sf::Vector2f(obj.first* scale));
            square.setFillColor(obj.second);
            window.draw(square);
        }

        window.display();

        if(clock.getElapsedTime().asMilliseconds() > delay){
            w.iterate();
            clock.restart(); 
        }
    }

    return 0;
}