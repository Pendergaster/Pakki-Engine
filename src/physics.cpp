#include <algorithm>
#include <new>
#include <physics.h>
#include <glm\glm\common.hpp>
#include <Texture.h>
#include <engine.h>

namespace PakkiPhysics
{


	enum class type
	{
		simulateObj,
		staticObj
	};
	struct simulated
	{
		vec2 velocity;
		float mass;
	};

	struct Static
	{
		float friction;
	};

	struct object
	{
		type	t;
		vec2	pos;
		vec2	dim;
		union
		{
			simulated	m;
			Static		s;
		};
	};
	struct collisionTable
	{
		object* obj1;
		object* obj2;
	};
#define MAX_TREELEVEL 4
#define MAX_OBJECTAMOUNT 10
	struct tree
	{		
		unsigned int				level;
		dynamicArray<object*>		objects;
		tree*						treebuffer;
		vec2						pos;
		vec2						dim;
	};

	void create_new_node(tree* node, uint32_t level,const vec2& pos,const vec2& dim)
	{
		node->level = level;
		node->pos = pos;
		node->level = level;
		node->dim = dim;
		if(node->objects.data == NULL)
		{
			node->objects.init_array(20);
		}
		node->treebuffer == NULL;
	}
	void clear_tree(tree* node)
	{
		if(node->treebuffer)
		{
			for(uint32_t i = 0;i < 4;i++)
			{
				clear_tree(&node->treebuffer[i]);
			}
		}
		node->objects.clear_array();
		node->treebuffer = NULL;
	}
	void split_tree(tree* node,tree* allocator,uint32_t* allocatorstart)
	{
		float subWidht = node->dim.x / 2;
		float subHeight = node->dim.y / 2;

		node->treebuffer = &allocator[*allocatorstart];
		*allocatorstart += 4;

		create_new_node(&node->treebuffer[0], node->level + 1,
			vec2{ node->pos.x + subWidht,node->pos.y + subHeight },
			vec2{ subWidht,subHeight });
		create_new_node(&node->treebuffer[1], node->level + 1,
			vec2{ node->pos.x - subWidht,node->pos.y + subHeight },
			vec2{ subWidht,subHeight });
		create_new_node(&node->treebuffer[2], node->level + 1,
			vec2{ node->pos.x - subWidht,node->pos.y - subHeight },
			vec2{ subWidht,subHeight });
		create_new_node(&node->treebuffer[3], node->level + 1,
			vec2{ node->pos.x + subWidht,node->pos.y - subHeight },
			vec2{ subWidht,subHeight });
	}

	int get_index(tree* node, vec2 pos, vec2 dim)
	{
		int index = -1;

		//if true it can fit to bottom place
		bool bot = pos.y - dim.y  > node->pos.y - node->dim.y
			&& pos.y + dim.y <  node->pos.y;
		//if true it can fit to top place
		bool top = pos.y - dim.y  > node->pos.y
			&& pos.y + dim.y <  node->pos.y + node->dim.y;


		if (pos.x - dim.x  > node->pos.x - node->dim.x
			&& pos.x + dim.x <  node->pos.x)
		{
			if (top)
			{
				index = 1;
			}
			else if (bot)
			{
				index = 2;
			}
		}
		else if (pos.x - dim.x  > node->pos.x
			&& pos.x + dim.x <  node->pos.x + node->dim.x)
		{
			if (top)
			{
				index = 0;
			}
			else if (bot)
			{
				index = 3;
			}
		}
		return index;
	}

	void insert_to_tree(tree* node, object* obj,tree* allocator,uint32_t* allocatorsize)
	{
		if(node->treebuffer)
		{
			int index = get_index(node, obj->pos, obj->dim);
			if(index != -1)
			{
				insert_to_tree(&node->treebuffer[index], obj, allocator, allocatorsize);
				return;
			}
		}
		object** newObj = node->objects.get_new_item();
		(*newObj) = obj;
		
		if(node->objects.get_size() >= MAX_OBJECTAMOUNT && node->level < MAX_TREELEVEL)
		{
			if(node->treebuffer == NULL)
			{
				split_tree(node, allocator, allocatorsize);
			}
			int i = 0;
			while(i < node->objects.get_size())
			{
				int index = get_index(node, node->objects.data[i]->pos, node->objects.data[i]->dim);
				if(index != 1)
				{
					insert_to_tree(&node->treebuffer[index], obj, allocator, allocatorsize);
					node->objects.data[i] = NULL;
					node->objects.fast_remove(i);
				}
				else
				{
					i++;
				}
			}
		}

	}

	void get_collisions(tree* node,dynamicArray<object*>* buffer,object* obj)
	{
		int index = get_index(node, obj->pos, obj->dim);
		if(index != -1 && node->treebuffer != NULL)
		{
			get_collisions(node, buffer, obj);
		}
		for(uint32_t i = 0; i < node->objects.get_size();i++)
		{
			object** newobj = buffer->get_new_item();
			*newobj = node->objects.data[i];
		}
	}

	/***************************SCENE STUFF****************************/

    static uint32_t testID = 0;
    static object* player = NULL;
    void init_scene(worldScene* scene,vec2 worldCentrePos,vec2 worldDimensions, uint32_t texID)
    {
        constexpr uint32_t poolamount = 1 + 4 * 4 * 4;
        memset(scene, 0, sizeof *scene);
        scene->treeAllocator = (tree*)malloc(sizeof(tree) * poolamount);
        memset(scene->treeAllocator, 0, sizeof(tree)*poolamount);
        scene->objectAllocator.init_pool();
        scene->objects.init_array(100);
        scene->updateobjects.init_array(50);
        scene->treeAllocatorSize = 1;
        scene->worldCentrePos = worldCentrePos;
        create_new_node(scene->treeAllocator, 0, worldCentrePos, worldDimensions);
        testID = texID;

        object* Terrain = scene->objectAllocator.new_item();
        object* Box = scene->objectAllocator.new_item();

        Terrain->pos = vec2{ 100,100 };
        Terrain->dim = vec2{ 30,30 };
        Terrain->t = type::staticObj;
        Terrain->s.friction = 0.90f;


        Box->dim = vec2{ 5,5 };
        Box->pos = vec2{ 100,180 };
        Box->t = type::simulateObj;
        Box->m.mass = 1;
        Box->m.velocity = vec2{ 0,0 };
        player = Box;

        object* Box2 = scene->objectAllocator.new_item();
        Box2->dim = vec2{ 5,5 };
        Box2->pos = vec2{ 100 ,180  + 20};
        Box2->t = type::simulateObj;
        Box2->m.mass = 0.5f;
        Box2->m.velocity = vec2{ 0,0 };


        object* Terrain2 = scene->objectAllocator.new_item();
        Terrain2->pos = vec2{ 100 - 60,100+ 60 };
        Terrain2->dim = vec2{ 30,30 };
        Terrain2->t = type::staticObj;
        Terrain2->s.friction = 0.90f;


        object* Terrain3 = scene->objectAllocator.new_item();
        Terrain3->pos = vec2{ 100 + 60,100 + 60 };
        Terrain3->dim = vec2{ 30,30 };
        Terrain3->t = type::staticObj;
        Terrain3->s.friction = 0.90f;


        scene->objects.push_back(Terrain);
        scene->objects.push_back(Terrain2);
        scene->objects.push_back(Terrain3);
        scene->objects.push_back(Box);
        scene->objects.push_back(Box2);



        scene->gravity.y = -5.811f;
    }
    void dispose_scene(worldScene* scene)
    {
        constexpr uint32_t poolamount = 1 + 4 * 4 * 4;
        for (uint32_t i = 0; i < poolamount; i++)
        {
            if (scene->treeAllocator[i].objects.data)
            {
                scene->treeAllocator[i].objects.dispose_array();
            }
        }
        free(scene->treeAllocator);
        scene->treeAllocator = NULL;
        scene->objectAllocator.dispose_pool();
        scene->objects.dispose_array();
        scene->updateobjects.dispose_array();
    }
	bool AABB(const object* obj1, const object* obj2)
	{
		float disx = std::abs(obj1->pos.x - obj2->pos.x);
		float disY = std::abs(obj1->pos.y - obj2->pos.y);
		return disx < obj1->dim.x + obj2->dim.y && disY < obj1->dim.y + obj2->dim.y;
	}
	void update_objects(worldScene& scene,float dt, keys* k)
	{

		for(uint32_t i = 0; i < scene.objects.get_size();i++)
		{
			object* currentobj = scene.objects.data[i];
            if (currentobj == player)
            {
                if (k->arrowU)
                {
                    currentobj->m.velocity.y += 15.f*dt ;
                }
                if (k->arrowL)
                {
                    currentobj->m.velocity.x -= 15.f *dt;
                }
                if (k->arrowR)
                {
                    currentobj->m.velocity.x += 15.f*dt ;
                }
            }
            if(currentobj->t == type::simulateObj)
            {
                currentobj->m.velocity.x += scene.gravity.x *dt;
                currentobj->m.velocity.y += scene.gravity.y *dt;
				currentobj->pos = vec2{ currentobj->m.velocity.x + currentobj->pos.x , currentobj->m.velocity.y + currentobj->pos.y };
			}
		}
		scene.treeAllocatorSize = 1;
		for(int i = 0; i < scene.objects.get_size();i++)
		{
			insert_to_tree(scene.treeAllocator, scene.objects.data[i], scene.treeAllocator, &scene.treeAllocatorSize);
		}

		dynamicArray<collisionTable> collisiontable;
        dynamicArray<collisionTable> hardcollisiontable;

		dynamicArray<object*> collisionbuffer;
        collisionbuffer.init_array();
        hardcollisiontable.init_array();

        collisiontable.init_array();
		for(uint32_t i = 0; i < scene.objects.get_size();i++)
		{
			object* currentobj = scene.objects.data[i];
			get_collisions(scene.treeAllocator,&collisionbuffer , currentobj);

			for (int j = 0; j < collisionbuffer.get_size();j++)
			{
				object* collider = collisionbuffer.data[j];
				if (currentobj == collider) continue;
				if (AABB(currentobj,collider))//collides
				{
					bool insert = true;
                    dynamicArray<collisionTable>*  mytable;
                    if (collider->t == type::staticObj || currentobj->t == type::staticObj)
                    {
                        mytable = &hardcollisiontable;
                    }
                    else
                    {
                        mytable = &collisiontable;
                    }

					for (uint32_t jj = 0; jj < mytable->get_size(); jj++)
					{
						if (mytable->data[jj].obj1 == currentobj && collider == mytable->data[jj].obj2)
						{
							insert = false;
							break;
						}
					}
					if (insert)
					{
                            collisionTable* t = mytable->get_new_item();
                            t->obj1 = collider;
                            t->obj2 = currentobj;
					}
				}
			}
			collisionbuffer.clear_array();
		}
		clear_tree(scene.treeAllocator);
		collisionbuffer.dispose_array();
        //**new implementation**//
        for(uint32_t i = 0; i < collisiontable.get_size();i++)
        {
            object* currentobject = collisiontable.data[i].obj1;
            object* collider = collisiontable.data[i].obj2;         

            float collVel = sqrt(collider->m.velocity.x *collider->m.velocity.x + collider->m.velocity.y*collider->m.velocity.y);
            float currVel = sqrt(currentobject->m.velocity.x *currentobject->m.velocity.x + currentobject->m.velocity.y*currentobject->m.velocity.y);


            if (collVel > currVel)
            {
                std::swap(currentobject, collider);
            }

            //**curren id always moved acording to how much inside its*//
            float dimensionalX = collider->pos.x - currentobject->pos.x >= 0 ? (currentobject->dim.x + collider->dim.x)*-1 : currentobject->dim.x + collider->dim.x;
            float dimensionalY = collider->pos.y - currentobject->pos.y >= 0 ? (currentobject->dim.y + collider->dim.y)*-1 : currentobject->dim.y + collider->dim.y;
            vec2 supportforce = abs(collider->pos.x - currentobject->pos.x) > abs(collider->pos.y - currentobject->pos.y) ? vec2{ collider->pos.x - currentobject->pos.x + dimensionalX,
                0 } : vec2{ 0, collider->pos.y - currentobject->pos.y + dimensionalY };


            currentobject->pos.x += supportforce.x;
            currentobject->pos.y += supportforce.y;


            vec2 newColliderSpeed = abs(currentobject->pos.x - collider->pos.x) > abs(currentobject->pos.y - collider->pos.y) ? vec2{ currentobject->m.velocity.x / collider->m.mass,0 } :
                vec2{ 0 , currentobject->m.velocity.y / collider->m.mass };



            vec2 newCUrrenVel = abs(currentobject->pos.x - collider->pos.x) > abs(currentobject->pos.y - collider->pos.y) ? vec2{ collider->m.velocity.x / currentobject->m.mass ,0 } :
                vec2{ 0 , collider->m.velocity.y / currentobject->m.mass };



            collider->m.velocity = newColliderSpeed;
            currentobject->m.velocity = newCUrrenVel;
        }
		
        for(uint32_t i = 0; i < hardcollisiontable.get_size();i++)
        {
            object* currentobject = hardcollisiontable.data[i].obj1;
            object* collider = hardcollisiontable.data[i].obj2;

            if (currentobject->t == type::staticObj && collider->t == type::simulateObj)
            {
                std::swap(currentobject, collider);
            }

            if(currentobject->t == type::simulateObj)
            {
                float dimensionalX = collider->pos.x - currentobject->pos.x >= 0 ? (currentobject->dim.x + collider->dim.x)*-1 : currentobject->dim.x + collider->dim.x;
                float dimensionalY = collider->pos.y - currentobject->pos.y >= 0 ? (currentobject->dim.y + collider->dim.y)*-1 : currentobject->dim.y + collider->dim.y;
                vec2 supportforce = abs(collider->pos.x - currentobject->pos.x) > abs(collider->pos.y - currentobject->pos.y) ? vec2{ collider->pos.x - currentobject->pos.x + dimensionalX,
                    0 } : vec2{ 0, collider->pos.y - currentobject->pos.y + dimensionalY };



                currentobject->pos.x += supportforce.x;
                currentobject->pos.y += supportforce.y;
                currentobject->m.velocity.x += supportforce.x;
                currentobject->m.velocity.y += supportforce.y;

                currentobject->m.velocity.x *= collider->s.friction;
                currentobject->m.velocity.y *= collider->s.friction;
            }
        }

        
        
        
     
		collisiontable.dispose_array();
        hardcollisiontable.dispose_array();
        object** ite = scene.objects.data;
        (*ite)->dim;
	}
    void draw_objects(worldScene* scene, SpriteBatch* batch)
    {
        for (object** iterator = scene->objects.data; iterator != scene->objects.get_back() + 1; iterator++)
        {
            glm::vec4 destRec;
            destRec.x = (*iterator)->pos.x - (*iterator)->dim.x;
            destRec.y = (*iterator)->pos.y - (*iterator)->dim.y;
            destRec.z = (*iterator)->dim.x * 2;
            destRec.w = (*iterator)->dim.y * 2;
            glm::vec4 uv(0, 0, 1, 1);
            Color color{ (GLubyte)255, (GLubyte)255, (GLubyte)255,(GLubyte)255 };
            push_to_batch(batch, &destRec, &uv, testID, &color, 0, 0);
        }
    }
}