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
    std::vector<std::vector<Object*> > objects;
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
        health(150),
        codePos(0),
        generation(0) {
        type = "living";
        color = sf::Color::Blue;

        std::uniform_int_distribution<int> sdist(10, 50);
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
        acter->health -= 150;
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
        acter->health += 100;
        world->remove(this);
        return true;
    }

	~Healing() = default;
};


World::World(sf::Vector2<int> size) : 
    size(size),
    iteration(0),
    generation(0) {
    	objects.resize(size.x);
    	for(auto& col: objects)
    		col.assign(size.y, 0);

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
    while(objects[pos.x][pos.y])
        pos = {xdist(gen), ydist(gen)};

    return pos;
}

void World::add(Object* object) {
    counter[object->getType()]++;
    objects[object->position.x][object->position.y] = object;
}

void World::remove(Object* object) {
    counter[object->getType()]--;
    Object* removing = objects[object->position.x][object->position.y];
    objects[object->position.x][object->position.y] = 0;
    delete removing;
}

void World::move(Living* living, sf::Vector2<int> newpos) {
    newpos.x = ((std::abs(newpos.x) / size.x + 1) * size.x + newpos.x) % size.x;
    newpos.y = ((std::abs(newpos.y) / size.y + 1) * size.y + newpos.y) % size.y;

    Object* reacter = objects[newpos.x][newpos.y];
    if(!reacter || reacter->react(this, living)) {
    	objects[newpos.x][newpos.y] = living;
    	objects[living->position.x][living->position.y] = 0;
    	living->position = newpos;
    }
}

int World::getCount(std::string type) {
    return counter[type];
}

void World::iterate() {
    for(int x = 0; x < size.x; x++)
    	for(int y = 0; y < size.y; y++)
	        if(objects[x][y] && objects[x][y]->getType() == "living")
	            ((Living*)objects[x][y])->act(this);

    if(getCount("living") <= 20) {
        generation++;

        std::vector<Object*> tmp;
        for(int x = 0; x < size.x; x++)
    		for(int y = 0; y < size.y; y++) 
	            if(objects[x][y] && objects[x][y]->getType() == "living")
	                tmp.push_back(objects[x][y]);

        for(int i = 0; i < tmp.size(); i++)
            add(new Living(randomPos(), (Living*)tmp[i], generation));
    }

    if(getCount("poison") <= 200)
        randomSpawn<Poison>(10);

    if(getCount("healing") <= 200)
        randomSpawn<Healing>(10);

    iteration++;
}

std::vector<std::pair<sf::Vector2<int>, sf::Color> > World::getDrawable(){
    std::vector<std::pair<sf::Vector2<int>, sf::Color> > retval;
    for(int x = 0; x < size.x; x++)
    	for(int y = 0; y < size.y; y++)
    		if(objects[x][y])
        		retval.push_back(
        			std::make_pair(	objects[x][y]->position, 
        							objects[x][y]->color )
        			);
    return retval;
}

void World::printStat() {
    std::cout<<"#################################"<<std::endl;
    std::cout<<"Iteration "<<iteration<<std::endl;
    std::cout<<"Generation "<<generation<<std::endl;

    for(int x = 0; x < size.x; x++)
		for(int y = 0; y < size.y; y++) 
            if(objects[x][y] && objects[x][y]->getType() == "living") {
        		Living* l = (Living*)objects[x][y];
        		std::cout<<"Living of Generation "<<l->generation
        		<<" with "<<l->health<<" hp"<<std::endl;
        		std::cout<<"Code: "<<std::endl;
        		for(int j = 0; j < l->code.size(); j++)
        			std::cout<<l->code[j]<<';';
        		std::cout<<std::endl;
            }

    std::cout<<"#################################"<<std::endl;
}

World::~World() {
    for(int x = 0; x < size.x; x++)
		for(int y = 0; y < size.y; y++)
			if(objects[x][y])
				delete objects[x][y];
}

int main()
{
    sf::Vector2<int> size = {120, 90};
    const int scale = 10;
    int delay = 10;

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
                    delay = std::max(delay - 20, 0);
                    std::cout<<"Delay "<<delay<<std::endl;
                }
                if (event.key.code == sf::Keyboard::Right) {
                    delay += 20;
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