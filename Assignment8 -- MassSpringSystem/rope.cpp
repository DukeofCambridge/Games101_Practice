#include <iostream>
#include <vector>

#include "CGL/vector2D.h"

#include "mass.h"
#include "rope.h"
#include "spring.h"

namespace CGL {

    Rope::Rope(Vector2D start, Vector2D end, int num_nodes, float node_mass, float k, vector<int> pinned_nodes)
    {
        // TODO (Part 1): Create a rope starting at `start`, ending at `end`, and containing `num_nodes` nodes.
        for(int i=0; i<num_nodes; ++i) {
            Vector2D pos = start + (end - start) * ((double)i / ((double)num_nodes - 1.0));          
            masses.push_back(new Mass(pos, node_mass, false));
            // masses[i]->forces = Vector2D(0, 0);
        }

        for(int i=0; i<num_nodes-1; ++i) {
            springs.push_back(new Spring(masses[i], masses[i+1], k));
        }

        // Comment-in this part when you implement the constructor
        for (auto &i : pinned_nodes) {
           masses[i]->pinned = true;
        }
    }


    void Rope::simulateEuler(float delta_t, Vector2D gravity)
    {
        for (auto &s : springs)
        {
            // TODO (Part 2): Use Hooke's law to calculate the force on a node
            auto len = (s->m1->position - s->m2->position).norm();
            s->m1->forces += -s->k * (s->m1->position - s->m2->position) / len * (len - s->rest_length);
            s->m2->forces += -s->k * (s->m2->position - s->m1->position) / len * (len - s->rest_length); 
        }


        for (auto &m : masses)
        {
#pragma region Explicit Euler Method
            if (!m->pinned)
            {
                // TODO (Part 2): Add the force due to gravity, then compute the new velocity and position
                auto a = m->forces / m->mass + gravity;
                m->position += m->velocity * delta_t; // For explicit method
                m->velocity += a * delta_t; 

                // TODO (Part 2): Add global damping
                auto a = m->forces / m->mass + gravity - kd * m->velocity / m->mass;
                m->velocity += a * delta_t;
                m->position += m->velocity * delta_t;
        	}
#pragma endregion
#pragma region semi-implicit Euler Method
			if (!m->pinned)
            {
                // TODO (Part 2): Add the force due to gravity, then compute the new velocity and position
                auto a = m->forces / m->mass + gravity;                
                m->velocity += a * delta_t; 
                m->position += m->velocity * delta_t; // For semi-implicit method
                // TODO (Part 2): Add global damping
                auto a = m->forces / m->mass + gravity - kd * m->velocity / m->mass;
                m->velocity += a * delta_t;
                m->position += m->velocity * delta_t;
            }
#pragma endregion

            // Reset all forces on each mass
            m->forces = Vector2D(0, 0);
        }
    }

    void Rope::simulateVerlet(float delta_t, Vector2D gravity)
    {
        for (auto &s : springs)
        {
            // TODO (Part 3): Simulate one timestep of the rope using explicit Verlet （solving constraints)
            auto len = (s->m1->position - s->m2->position).norm();
            s->m1->forces += -s->k * (s->m1->position - s->m2->position) / len * (len - s->rest_length);
            s->m2->forces += -s->k * (s->m2->position - s->m1->position) / len * (len - s->rest_length);
        }

        for (auto &m : masses)
        {
            if (!m->pinned)
            {
                Vector2D temp_position = m->position;
                auto a = m->forces / m->mass + gravity;
                // TODO (Part 3.1): Set the new position of the rope mass
                m->position = temp_position + (temp_position - m->last_position) + a * delta_t * delta_t;
                m->last_position = temp_position;
                // TODO (Part 4): Add global Verlet damping
                double damping_factor = 0.00005;
                m->position = temp_position + (1 - damping_factor) * (temp_position - m->last_position) + a * delta_t * delta_t; 
                m->last_position = temp_position;
            }
            m->forces = Vector2D(0, 0);
        }
    }
}
