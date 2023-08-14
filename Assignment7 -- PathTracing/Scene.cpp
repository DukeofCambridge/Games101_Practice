//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

void Scene::buildSAH() {
    printf(" - Generating SAH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
//Vector3f Scene::castRay(const Ray &ray, int depth) const
//{
//    Intersection intersection = Scene::intersect(ray);
//    if (intersection.happened) {
//        Vector3f hitPoint = intersection.coords;
//        Vector3f N = intersection.normal; // normal
//        Material* m = intersection.m;
//        Vector3f L_dir(0.0), L_indir(0.0);
//#pragma region Light_direct
//        // Uniformly sample the light at x (pdf_light = 1 / A)
//        Intersection intersection_light;
//        float pdf_light;
//        sampleLight(intersection_light, pdf_light);
//        // Shoot a ray from p to x
//        Vector3f dir_p_x = (intersection_light.coords - hitPoint).normalized();
//        // Add a small vector along the normal direction to avoid being blocked by itself
//        Ray ray_p_x(hitPoint + EPSILON * N, dir_p_x);
//        // std::cout<<hitPoint.x<<" "<<hitPoint.y<<" "<<hitPoint.z<<std::endl;
//        // std::cout<<dir_p_x.x<<" "<<dir_p_x.y<<" "<<dir_p_x.z<<std::endl;
//        Intersection intersection_p_x = Scene::intersect(ray_p_x);
//        // If the ray is not blocked in the middle
//        if (intersection_p_x.happened && intersection_p_x.m->hasEmission()) {
//            Vector3f NN = intersection_p_x.normal;
//            L_dir = intersection_p_x.m->m_emission * m->eval(ray.direction, dir_p_x, N) * dotProduct(dir_p_x, N) * dotProduct(-dir_p_x, NN) / (intersection_p_x.distance* intersection_p_x.distance) / pdf_light;
//        }
//#pragma endregion
//#pragma region Light_indirect
//        if (get_random_float() <= RussianRoulette) {
//            // Trace a ray r(p, wi)
//            Vector3f dir_i = m->sample(ray.direction, N).normalized();
//            Ray ray_p_diri(hitPoint, dir_i);
//            Intersection intersection_p_diri = Scene::intersect(ray_p_diri);
//
//            // If ray r hit a non-emitting object at q
//            if (intersection_p_diri.happened && !intersection_p_diri.m->hasEmission()) {
//                L_indir = castRay(ray_p_diri, depth + 1) * m->eval(ray.direction, dir_i, N) * dotProduct(dir_i, N) / m->pdf(ray.direction, dir_i, N) / RussianRoulette;
//            }
//        }
//#pragma endregion
//        return m->getEmission() + L_dir + L_indir;
//    }
//    return Vector3f(0, 0, 0);
//}
Vector3f Scene::castRay(const Ray& ray, int depth) const
{
	Intersection inter = intersect(ray);

	if (inter.happened)
	{
		if (inter.m->hasEmission())
		{
			if (depth == 0)
			{
				return inter.m->getEmission();
			}
			else return Vector3f(0, 0, 0);
		}
		Vector3f L_dir(0, 0, 0);
		Vector3f L_indir(0, 0, 0);
#pragma region Light_direct
		Intersection lightInter;
		float pdf_light = 0.0f;
		sampleLight(lightInter, pdf_light);

		// object surface normal
		auto& N = inter.normal;
		// light surface normal
		auto& NN = lightInter.normal;

		auto& objPos = inter.coords;
		auto& lightPos = lightInter.coords;

		auto diff = lightPos - objPos;
		auto lightDir = diff.normalized();
		float lightDistance = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

		Ray light(objPos, lightDir);
		Intersection light2obj = intersect(light);

		if (light2obj.happened && (light2obj.coords - lightPos).norm() < 1e-2)
		{
			Vector3f f_r = inter.m->eval(ray.direction, lightDir, N);
			L_dir = lightInter.emit * f_r * dotProduct(lightDir, N) * dotProduct(-lightDir, NN) / lightDistance / pdf_light;
		}
#pragma endregion
#pragma region Light_indirect
		if (get_random_float() < RussianRoulette)
		{
			Vector3f nextDir = inter.m->sample(ray.direction, N).normalized();

			Ray nextRay(objPos, nextDir);
			Intersection nextInter = intersect(nextRay);
			if (nextInter.happened && !nextInter.m->hasEmission())
			{
				float pdf = inter.m->pdf(ray.direction, nextDir, N);
				Vector3f f_r = inter.m->eval(ray.direction, nextDir, N);
				L_indir = castRay(nextRay, depth + 1) * f_r * dotProduct(nextDir, N) / pdf / RussianRoulette;
			}
		}
#pragma endregion
		return L_dir + L_indir;
	}

	return Vector3f(0, 0, 0);
}