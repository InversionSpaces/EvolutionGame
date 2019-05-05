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

const sf::Vector2<int> moves[] = {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}};

class Object;
class Living;
class Posion;

class World;

class Object {
    friend class World;
private:
    sf::Vector2<int> position;
protected:
    sf::Color color;
public: 
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

class World {
private:
    std::vector<Object*> objects;
    std::vector<Living*> livings;

    sf::Vector2<int> size;
public:
    World(sf::Vector2<int> size);

    void remove(Object* object);
    void remove(Living* living);

    void move(Living* living, sf::Vector2<int> newpos);

    bool checkPos(sf::Vector2<int> pos);

    void iterate();

    std::vector<std::pair<sf::Vector2<int>, sf::Color> > getDrawable();

    ~World();
};


Object::Object(sf::Vector2<int> position) : 
    position(position) { }

bool Object::react(World* world, Living* acter) {
    return true;
}

Object::~Object() { }



std::random_device dev;
std::mt19937 gen(dev());

Living::Living(sf::Vector2<int> position) : 
    Object(position),
    health(100),
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

    world->remove(this);
    if(acter->health <= 0)
        world->remove(acter);

    return false;
}

Poison::~Poison() { }


World::World(sf::Vector2<int> size) : size(size) {
    std::uniform_int_distribution<int> xdist(0, size.x);
    std::uniform_int_distribution<int> ydist(0, size.y);

    for(int i = 0; i < 8; i++){
        sf::Vector2<int> newpos = {xdist(gen), ydist(gen)};
        while(checkPos(newpos))
            newpos = {xdist(gen), ydist(gen)};
        livings.push_back(new Living(newpos));
    }
}

bool World::checkPos(sf::Vector2<int> pos){
    return (std::find_if(livings.begin(), livings.end(), 
            [pos] (Living* l) { 
                    return l->position == pos;
                }) != livings.end()) || 
            (std::find_if(objects.begin(), objects.end(), 
            [pos] (Object* o) { 
                    return o->position == pos;
                }) != objects.end());
}


void World::remove(Object* object){
    auto it = std::find(objects.begin(), objects.end(), object);
    if (it != objects.end()){
        delete (*it);
        objects.erase(it);
    }
}


void World::remove(Living* living){
    auto it = std::find(livings.begin(), livings.end(), living);
    if (it != livings.end()){
        delete (*it);
        livings.erase(it);
    }
}


void World::iterate(){
    for(int i = 0; i < livings.size(); i++){
        livings[i]->act(this);
    }
}

void World::move(Living* living, sf::Vector2<int> newpos){
    newpos += living->position;

    newpos.x = ((std::abs(newpos.x) / size.x + 1) * size.x + newpos.x) % size.x;
    newpos.y = ((std::abs(newpos.y) / size.y + 1) * size.y + newpos.y) % size.y;

    auto ito = std::find_if(objects.begin(), objects.end(), [newpos] (Object* obj) { return obj->position == newpos; });
    if(ito != objects.end()){
        if((*ito)->react(this, living)) {
            living->position = newpos;
        }
        return;
    }

    auto itl = std::find_if(livings.begin(), livings.end(), [newpos] (Living* liv) { return liv->position == newpos; });
    if(itl != livings.end()){
        if((*itl)->react(this, living)){
            living->position = newpos;
        }
        return;
    }

    living->position = newpos;
}

std::vector<std::pair<sf::Vector2<int>, sf::Color> > World::getDrawable(){
    std::vector<std::pair<sf::Vector2<int>, sf::Color> > retval(objects.size() + livings.size());
    for(int i = 0; i < objects.size(); i++)
        retval[i] = std::make_pair(objects[i]->position, objects[i]->color);
    for(int i = 0; i < livings.size(); i++)
        retval[i + objects.size()] = std::make_pair(livings[i]->position, livings[i]->color);
    return retval;
}

World::~World() {
    for(int i = 0; i < livings.size(); i++)
        delete livings[i];
    for(int i = 0; i < objects.size(); i++)
        delete objects[i];
}

int main()
{
    sf::Vector2<int> size = {20, 20};
    const int scale = 25;
    const int delay = 100;

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