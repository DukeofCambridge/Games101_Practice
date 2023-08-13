#include <algorithm>
#include <cassert>
#include "BVH.hpp"

BVHAccel::BVHAccel(std::vector<Object*> p, int maxPrimsInNode,
                   SplitMethod splitMethod)
    : maxPrimsInNode(std::min(255, maxPrimsInNode)), splitMethod(splitMethod),
      primitives(std::move(p))
{
    time_t start, stop;
    time(&start);
    if (primitives.empty())
        return;

    root = recursiveBuild(primitives);
    //root = recursiveBuild_SAH(primitives);

    time(&stop);
    double diff = difftime(stop, start);
    int hrs = (int)diff / 3600;
    int mins = ((int)diff / 60) - (hrs * 60);
    int secs = (int)diff - (hrs * 3600) - (mins * 60);

    printf("\rBVH Generation complete: \nTime Taken: %i hrs, %i mins, %i secs\n\n", hrs, mins, secs);
    //printf("\rSAH Generation complete: \nTime Taken: %i hrs, %i mins, %i secs\n\n", hrs, mins, secs);
}

BVHBuildNode* BVHAccel::recursiveBuild(std::vector<Object*> objects)
{
    BVHBuildNode* node = new BVHBuildNode();

    // Compute bounds of all primitives in BVH node
    Bounds3 bounds;
    for (int i = 0; i < objects.size(); ++i)
        bounds = Union(bounds, objects[i]->getBounds());
    if (objects.size() == 1) {
        // Create leaf _BVHBuildNode_
        node->bounds = objects[0]->getBounds();
        node->object = objects[0];
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }
    else if (objects.size() == 2) {
        node->left = recursiveBuild(std::vector{objects[0]});
        node->right = recursiveBuild(std::vector{objects[1]});

        node->bounds = Union(node->left->bounds, node->right->bounds);
        return node;
    }
    else {
        Bounds3 centroidBounds;
        for (int i = 0; i < objects.size(); ++i)
            centroidBounds =
                Union(centroidBounds, objects[i]->getBounds().Centroid());
        int dim = centroidBounds.maxExtent();
        switch (dim) {
        case 0:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().x <
                       f2->getBounds().Centroid().x;
            });
            break;
        case 1:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().y <
                       f2->getBounds().Centroid().y;
            });
            break;
        case 2:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().z <
                       f2->getBounds().Centroid().z;
            });
            break;
        }

        auto beginning = objects.begin();
        auto middling = objects.begin() + (objects.size() / 2);
        auto ending = objects.end();

        auto leftshapes = std::vector<Object*>(beginning, middling);
        auto rightshapes = std::vector<Object*>(middling, ending);

        assert(objects.size() == (leftshapes.size() + rightshapes.size()));

        node->left = recursiveBuild(leftshapes);
        node->right = recursiveBuild(rightshapes);

        node->bounds = Union(node->left->bounds, node->right->bounds);
    }

    return node;
}

BVHBuildNode* BVHAccel::recursiveBuild_SAH(std::vector<Object*> objects)
{
    BVHBuildNode* node = new BVHBuildNode();
    // std::cout<<objects.size()<<std::endl;

    // Compute bounds of all primitives in SAH node
    Bounds3 bounds;
    for (int i = 0; i < objects.size(); ++i)
        bounds = Union(bounds, objects[i]->getBounds());
    if (objects.size() == 1) {
        // Create leaf _SAHBuildNode_
        node->bounds = objects[0]->getBounds();
        node->object = objects[0];
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }
    else if (objects.size() == 2) {
        node->left = recursiveBuild_SAH(std::vector{objects[0]});
        node->right = recursiveBuild_SAH(std::vector{objects[1]});

        node->bounds = Union(node->left->bounds, node->right->bounds);
        return node;
    }
    else {
        Bounds3 centroidBounds;
        for (int i = 0; i < objects.size(); ++i)
            centroidBounds =
            Union(centroidBounds, objects[i]->getBounds().Centroid());
        int dim = centroidBounds.maxExtent();
        switch (dim) {
        case 0:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().x <
                    f2->getBounds().Centroid().x;
                });
            break;
        case 1:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().y <
                    f2->getBounds().Centroid().y;
                });
            break;
        case 2:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().z <
                    f2->getBounds().Centroid().z;
                });
            break;
        }

        int B_size = 10;
        int minCostIdx = 1;
        float minCost = std::numeric_limits<float>::infinity();
        float SC = bounds.SurfaceArea();
        // float SC = centroidBounds.SurfaceArea();

        auto beginning = objects.begin();
        auto ending = objects.end();

        for (int i = 1; i < B_size; ++i) {// search minCost and minCostIdx
            auto middling = objects.begin() + std::max(1, (int)(objects.size() * i / B_size));

            auto leftshapes = std::vector<Object*>(beginning, middling);
            auto rightshapes = std::vector<Object*>(middling, ending);

            assert(objects.size() == (leftshapes.size() + rightshapes.size()));

            Bounds3 leftbounds, rightbounds;

            for (int j = 0; j < leftshapes.size(); ++j) {
                leftbounds = Union(leftbounds, leftshapes[j]->getBounds());
                // leftbounds = Union(leftbounds, leftshapes[j]->getBounds().Centroid());
            }

            for (int j = 0; j < rightshapes.size(); ++j) {
                rightbounds = Union(rightbounds, rightshapes[j]->getBounds());
                // rightbounds = Union(rightbounds, rightshapes[j]->getBounds().Centroid());
            }

            float SA = leftbounds.SurfaceArea();
            float SB = rightbounds.SurfaceArea();
            float c_A_B = leftshapes.size() * SA / SC + rightshapes.size() * SB / SC + 0.125f;

            if (c_A_B < minCost) {
                minCost = c_A_B;
                minCostIdx = i;
            }
        }

        auto middling_minCost = objects.begin() + std::max(1, ((int)objects.size() * minCostIdx / B_size));
        auto leftshapes_minCost = std::vector<Object*>(beginning, middling_minCost);
        auto rightshapes_minCost = std::vector<Object*>(middling_minCost, ending);

        assert(objects.size() == (leftshapes_minCost.size() + rightshapes_minCost.size()));

        // std::cout<<leftshapes_minCost.size()<<" "<<rightshapes_minCost.size()<<std::endl;
        node->left = recursiveBuild_SAH(leftshapes_minCost);
        node->right = recursiveBuild_SAH(rightshapes_minCost);

        node->bounds = Union(node->left->bounds, node->right->bounds);
    }

    return node;
}

Intersection BVHAccel::Intersect(const Ray& ray) const
{
    Intersection isect;
    if (!root)
        return isect;
    isect = BVHAccel::getIntersection(root, ray);
    return isect;
}

Intersection BVHAccel::getIntersection(BVHBuildNode* node, const Ray& ray) const
{
    // Traverse the BVH to find intersection
    Intersection isect;
    if (!node->bounds.IntersectP(ray, ray.direction_inv, std::array<int, 3> {ray.direction.x > 0, ray.direction.y > 0, ray.direction.z > 0}))
        return isect;
    if (node->object != nullptr) // leaf node
        return node->object->getIntersection(ray);

    // ray-box intersected but there is no object in the node, then search the internal nodes
    Intersection isect_left, isect_right;
    isect_left = getIntersection(node->left, ray);
    isect_right = getIntersection(node->right, ray);

    return isect_left.distance <= isect_right.distance ? isect_left : isect_right;
}