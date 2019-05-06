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

class World {
public:
    int iteration, generation;
    std::vector<Object*> objects;
    std::map<std::string, int> counter;
    sf::Vector2<int> size;

    World(sf::Vector2<int> size);

    template<typename T>
    void randomSpawn(int count);

    void add(Object* object);

    void remove(Object* object);

    void move(Living* living, sf::Vector2<int> newpos);

    bool checkPos(sf::Vector2<int> pos);

    int getCount(std::string type);

    sf::Vector2<int> randomPos();

    void iterate();

    void printStat();

    std::vector<std::pair<sf::Vector2<int>, sf::Color> > getDrawable();

    ~World();
};

class Object {
public:
    std::string type;

    sf::Vector2<int> position;
    sf::Color color;
 
    Object(sf::Vector2<int> position) : 
        position(position), 
        type("object"),
        color(sf::Color::Transparent) { }

    std::string getType() {
        return type;
    }

    virtual bool react(World* world, Living* acter) {
        return true;
    }

    ~Object() = default;
};

class Living : public Object {
public:
    int generation;
    int health;
    std::vector<int> code;
    int codePos;

    Living(sf::Vector2<int> position) :
        Object(position),
        health(50),
        codePos(0),
        generation(0) {
        type = "living";
        color = sf::Color::Blue;

        std::uniform_int_distribution<int> sdist(10, 100);
        std::uniform_int_distribution<int> cdist(0, 7);

        for(int i = 0; i < sdist(gen); i++)
            code.push_back(cdist(gen));
    }

    Living(sf::Vector2<int> position, Living* parent, int generation) :
        Object(position),
        health(50),
        codePos(0),
        generation(generation) {
        type = "living";
        color = sf::Color::Blue;

        std::uniform_int_distribution<int> sdist(-2, 2);
        std::uniform_int_distribution<int> cdist(0, 7);

        code.resize(std::max(int(parent->code.size()) + sdist(gen), 1));
        int part = std::min(int(code.size()), int(parent->code.size()));

        for(int i = 0; i < part; i++)
            code[i] = parent->code[i];

        for(int i = part; i < code.size(); i++)
            code[i] = cdist(gen);

        std::uniform_int_distribution<int> rdist(0, part / 10);
        std::uniform_int_distribution<int> pdist(0, part - 1);

        int count = rdist(gen);
        for(int i = 0; i < count; i++)
            code[pdist(gen)] = cdist(gen);
    }

    void act(World* world) {
        health--;
        world->move(this, position + moves[code[codePos]]);
        codePos = (codePos + 1) % code.size();
        if(health <= 0) world->remove(this);
    }

    bool react(World* world, Living* acter) {
        return false;
    }

    ~Living() = default;
};

class Poison : public Object {
public:
    Poison(sf::Vector2<int> position) :
        Object(position) {
        type = "poison";
        color = sf::Color::Red;
    }

    bool react(World* world, Living* acter) {
        acter->health -= 5;
        world->remove(this);
        return true;
    }

    ~Poison() = default;
};

class Wall : public Object {
public:
	Wall(sf::Vector2<int> position) :
        Object(position) {
        type = "wall";
        color = sf::Color::White;
    }

	bool react(World* world, Living* acter) {
        return false;
    }

	~Wall();
};

class Healing : public Object {
public:
	Healing(sf::Vector2<int> position) : 
        Object(position) {
        type = "healing";
        color = sf::Color::Green;
    }

	bool react(World* world, Living* acter) {
        acter->health += 5;
        world->remove(this);
        return true;
    }

	~Healing() = default;
};


World::World(sf::Vector2<int> size) : 
    size(size),
    iteration(0),
    generation(0) {
        randomSpawn<Living>(40);
        randomSpawn<Wall>(1000);
        randomSpawn<Healing>(200);
        randomSpawn<Poison>(200);
}

template<typename T>
void World::randomSpawn(int count) {
    for(int i = 0; i < count; i++) {
        add(new T(randomPos()));
    }
}

sf::Vector2<int> World::randomPos() {
    std::uniform_int_distribution<int> xdist(0, size.x - 1);
    std::uniform_int_distribution<int> ydist(0, size.y - 1);

    sf::Vector2<int> pos = {xdist(gen), ydist(gen)};
    while(checkPos(pos))
        pos = {xdist(gen), ydist(gen)};
    return pos;
}

void World::add(Object* object) {
    counter[object->getType()]++;
    objects.push_back(object);
}

void World::remove(Object* object) {
    counter[object->getType()]--;
    auto it = std::find(objects.begin(), objects.end(), object);
    delete (*it);
    objects.erase(it);
}

void World::move(Living* living, sf::Vector2<int> newpos) {
    newpos.x = ((std::abs(newpos.x) / size.x + 1) * size.x + newpos.x) % size.x;
    newpos.y = ((std::abs(newpos.y) / size.y + 1) * size.y + newpos.y) % size.y;

    for(int i = 0; i < objects.size(); i++)
        if(objects[i]->position == newpos){
            if(objects[i]->react(this, living))
                living->position = newpos;
            return;
        }

    living->position = newpos;
}

bool World::checkPos(sf::Vector2<int> pos) {
    return  std::find_if(objects.begin(), objects.end(), 
            [pos] (Object* o) { return o->position == pos; })
            != objects.end();
}

int World::getCount(std::string type) {
    return counter[type];
}

void World::iterate() {
    for(int i = 0; i < objects.size(); i++) 
        if(objects[i]->getType() == "living")
            ((Living*)objects[i])->act(this);

    if(getCount("living") <= 20) {
        generation++;

        std::vector<Object*> tmp;
        for(int i = 0; i < objects.size(); i++) {
            if(objects[i]->getType() == "living")
                tmp.push_back(objects[i]);
        }
        for(int i = 0; i < tmp.size(); i++) {
            add(new Living(randomPos(), (Living*)tmp[i], generation));
        }
    }

    if(getCount("poison") <= 100)
        randomSpawn<Poison>(100);

    if(getCount("healing") <= 100)
        randomSpawn<Healing>(100);

    iteration++;
}

std::vector<std::pair<sf::Vector2<int>, sf::Color> > World::getDrawable(){
    std::vector<std::pair<sf::Vector2<int>, sf::Color> > retval(objects.size());
    for(int i = 0; i < objects.size(); i++)
        retval[i] = std::make_pair(objects[i]->position, objects[i]->color);
    return retval;
}

void World::printStat() {
    std::cout<<"--------------"<<std::endl;
    std::cout<<"Iteration "<<iteration<<std::endl;
    std::cout<<"Generation "<<generation<<std::endl;
    for(int i = 0; i < objects.size(); i++) {
        if(objects[i]->getType() == "living"){
            Living* l = (Living*)objects[i];
            std::cout<<"Living of Generation "<<l->generation
            <<" with "<<l->health<<" hp"<<std::endl;
        }
    }
    std::cout<<"--------------"<<std::endl;
}

World::~World() {
    for(int i = 0; i < objects.size(); i++)
        delete objects[i];
}

int main()
{
    sf::Vector2<int> size = {120, 90};
    const int scale = 10;
    int delay = 1000;

    World w(size);

    sf::RenderWindow window(sf::VideoMode(size.x * scale, size.y * scale), "Evolution");
    sf::Clock clock;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Up) {
                    w.printStat();
                }
                if (event.key.code == sf::Keyboard::Left) {
                    delay = std::max(delay - 100, 0);
                    std::cout<<"Delay "<<delay<<std::endl;
                }
                if (event.key.code == sf::Keyboard::Right) {
                    delay += 100;
                    std::cout<<"Delay "<<delay<<std::endl;
                }

            }
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