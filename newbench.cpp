#include <cstdlib>
#include <iostream>
#include <mutex>
#include <memory>
#include <chrono>
#include <execution>
#include <algorithm>
#include <thread>

using namespace std::chrono;
using namespace std;

#include <stdlib.h>
#include <cmath>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/fast_exponential.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

struct bbox_t {
    glm::vec3 min;
    glm::vec3 max;
    bool unset = true;

    bbox_t& operator+=(const bbox_t& b) {
        *this += b.min;
        *this += b.max;
        return *this;
    }
    bbox_t& operator+=(const glm::vec3& p) {
        if (unset) {
            min = max = p;
            unset = false;
        }
        else {
            min = glm::min(min, p);
            max = glm::max(max, p);
        }
        return *this;
    }

    glm::vec3 center(void) const {
        return (min + max) / 2.0f;
    }

    void  print(void) const {
        cout << "bbox[" << glm::to_string(min) << "," << glm::to_string(max) << "]";
    }
};

struct Node {
    glm::vec3 position;
    float weight;
    glm::vec3 velocity;
    float unused;
};

#include <vector>
class otNode
{
private:
    enum type_e { LEAF, NODE };
    type_e type;
    int threshold;
    vector<Node*> nodes;
    vector<otNode*> children;
    std::mutex lock;

    glm::vec3 center;

    float weight = 0;
    bbox_t bbox;
    float bboxSize = 0.0;
    glm::vec3 baryCenter;

public:
    otNode(int t = 4) : threshold(t), center(0), baryCenter(0), type(LEAF)
    { }
    ~otNode() {
        for (auto c : children) {
            delete c;
        }
    }

	void updateStats(int level) {
		if (type == NODE) {
			if (level) {
				std::for_each(std::execution::par_unseq, children.begin(), children.end(),
					[&](auto& c) {
						c->updateStats(level - 1);
					});
			}
			else {
				for (auto& c : children) {
					c->updateStats(level - 1);
				}
			}
			for (auto c : children) {
				c->updateStats(level - 1);
            }
            for (auto& c : children) {
                if (c->type == LEAF && c->nodes.empty())
                    continue;
                bbox += c->bbox;
                weight += c->weight;
                baryCenter += c->baryCenter * float(c->weight);
            }
            baryCenter /= weight;
            bboxSize = ((bbox.max.x - bbox.min.x) +
                (bbox.max.y - bbox.min.y) +
                (bbox.max.z - bbox.min.z)) / 3.0;
        }
        else {
            if (!nodes.empty())
            {
                for (auto n : nodes) {
                    bbox += n->position;
                    weight += n->weight;
                    baryCenter += n->position * n->weight;
                }
                baryCenter /= weight;
            }
        }
    }

    void print(int i = 0, bool leafNodes = false) {
        if (type == LEAF) {
            if (nodes.empty())
                return;
            for (int c = 0; c < i; ++c) {
                cout << " ";
            }
            cout << "Level " << i << " LEAF " << nodes.size() << " ";
            bbox.print();
            cout << " BC " << glm::to_string(baryCenter) << " W " << weight
                << " size " << bboxSize << endl;
            if (leafNodes) {
                for (auto& n : nodes) {
                    for (int c = 0; c < i + 1; ++c) {
                        cout << " ";
                    }
                    cout << glm::to_string(n->position) << " " << n->weight << endl;
                }
            }
        }
        else {
            for (int c = 0; c < i; ++c) {
                cout << " ";
            }
            cout << "Level " << i << " " << "NODE ";
            bbox.print();
            cout << " BC " << glm::to_string(baryCenter) << " W " << weight
                << " size " << bboxSize << endl;
        }
        if (type == NODE) {
            for (auto& c : children) {
                c->print(i + 1, leafNodes);
            }
        }
    }
    glm::vec3 force(glm::vec3& p1, float m1, glm::vec3& p2, float m2) {

        double d = glm::distance(p2, p1);

        // m1*m2 / r^2
        float force = (m1 * m2) / ((d * d) + sqrt(m1 + m2));

        glm::vec3 direction = glm::normalize(p2 - p1);
        return direction * force;
        /*
          double d = glm::distance(p2, p1);
          glm::vec3 direction = glm::normalize(p2 - p1);
          return direction * (float)(m2/(d*d+1.0));
        */
    }
    void insert(Node* n) {
        if (type == NODE) {
            glm::vec3 cmp = glm::lessThan(n->position, center);
            int oct = cmp.x * 2 + cmp.y * 1 + cmp.z * 4;
            children[oct]->insert(n);
        }
        else {
            // LEAF
            lock.lock();
            if (type == NODE) {
                lock.unlock();
                this->insert(n);
            }
            else {
                nodes.push_back(n);
                if (nodes.size() == threshold) {
                    // Initialize child nodes.
                    for (int i = 0; i < 8; ++i) {
                        children.push_back(new otNode(threshold));
                    }

                    for (auto& n : nodes) {
                        center += n->position;
                    }
                    center /= nodes.size();
                    type = NODE;
                    for (auto& n : nodes) {
                        this->insert(n);
                    }
                    nodes.clear();
                }
                lock.unlock();
            } // if (nodes.size() == threshold)
        } // else
    } // void insert(Node* n)

    glm::vec3 calcForce(Node* n, float theta) {
        glm::vec3 f(0);
        if (type == NODE) {
            float distance = glm::distance(n->position, baryCenter);
            if (distance / bboxSize > theta) {
                f = force(n->position, n->weight, baryCenter, weight);
            }
            else {
                for (auto& c : children) {
                    f += c->calcForce(n, theta);
                }
            }
        }
        else {
            for (auto& c : nodes) {
                if (c == n) continue;
                f += force(n->position, n->weight, c->position, c->weight);
            }
        }
        return f;
    }
};

int main(int argc, char* argv[]) {
    unsigned int numNodes = 1000;
    unsigned int threshold = 8;
    float theta = 0.7;
    unsigned int iterations = 60;
    bool printTree = false;

    for (int cnt = 1; cnt < argc; cnt++)
    {
        if (strcmp(argv[cnt], "-n") == 0)
            numNodes = atoi(argv[cnt + 1]);
        if (strcmp(argv[cnt], "-t") == 0)
            threshold = atoi(argv[cnt + 1]);
        if (strcmp(argv[cnt], "-e") == 0)
            theta = atof(argv[cnt + 1]);
        if (strcmp(argv[cnt], "-i") == 0)
            iterations = atoi(argv[cnt + 1]);
        if (strcmp(argv[cnt], "-p") == 0)
            printTree = true;
    }

    std::cout << "nodes: " << numNodes << " threshold: " << threshold
        << " theta: " << theta << " iterations: " << iterations
        << std::endl;
    vector<Node> nodes(numNodes);

    // Initialize positions in a ball.
    float r = 40 * std::cbrt(numNodes);
    for (auto& n : nodes) {
        //	n.position = glm::ballRand(r);
        n.position = glm::vec3(glm::diskRand(r), 1.0);
        n.weight = glm::fastExp(glm::linearRand(0.0, 6.0));
        n.velocity = glm::vec3(0.0f);
    }


    auto start_iterations = high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto start = high_resolution_clock::now();
        otNode root(threshold);
        std::cout << " root "
            << duration_cast<microseconds>(high_resolution_clock::now() - start).count() / 1000000.0
            << std::endl;
        start = high_resolution_clock::now();

        std::for_each(std::execution::par_unseq, nodes.begin(), nodes.end(),
            [&](auto& n) {
                root.insert(&n);
            });
        std::cout << " insert "
            << duration_cast<microseconds>(high_resolution_clock::now() - start).count() / 1000000.0
            << std::endl;
        start = high_resolution_clock::now();

        root.updateStats(1);
        std::cout << " stats "
            << duration_cast<microseconds>(high_resolution_clock::now() - start).count() / 1000000.0
            << std::endl;
        start = high_resolution_clock::now();

        if (printTree)
            root.print();
        // Calculate forces.

        std::for_each(std::execution::par_unseq, nodes.begin(), nodes.end(),
            [&](auto& n) {
                n.velocity += root.calcForce(&n, theta);
            });
        std::cout << " forces "
            << duration_cast<microseconds>(high_resolution_clock::now() - start).count() / 1000000.0
            << std::endl;
        start = high_resolution_clock::now();

        for (auto& n : nodes) {
            n.position += n.velocity;
        }
        std::cout << " update "
            << duration_cast<microseconds>(high_resolution_clock::now() - start).count() / 1000000.0
            << std::endl;

    }
    std::cout << iterations << " iterations in "
        << duration_cast<microseconds>(high_resolution_clock::now() - start_iterations).count() / 1000000.0
        << std::endl;

    return 0;
}
