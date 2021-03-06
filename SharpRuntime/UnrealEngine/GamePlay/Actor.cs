﻿using System;
using System.Runtime.CompilerServices;


namespace UnrealEngine
{
    public class Actor : UObject
    {
        public Actor()
        {
        }

        public Actor(IntPtr handler)
        {
            NativeHandler = handler;
        }
        
        /// <summary>
        /// 设置跟获取是否隐藏
        /// </summary>
        public bool HiddenInGame
        {
            get { return _GetHiddenInGame(NativeHandler); }
            set { _SetHiddenInGame(NativeHandler, value); }
        }

        /// <summary>
        /// 获取场景的根组件
        /// </summary>
        private SceneComponent m_Root;
        public SceneComponent Root
        {
            get
            {
                if (m_Root == null)
                {
                    var handler = _GetSceneComponent(NativeHandler);
                    if (handler == null)
                    {
                        m_Root = null;
                    }
                    else
                    {
                        m_Root = new SceneComponent();
                        m_Root.NativeHandler = handler;
                    }
                }
                return m_Root;
            }
            set { _SetSceneComponent(NativeHandler, value.NativeHandler); }
        }

        /// <summary>
        /// 获取这个Actor上的Sequencer
        /// </summary>
        public Sequencer Sequencer
        {
            get
            {
                var sequencer = new Sequencer();
                sequencer.NativeHandler = _GetSequencer(NativeHandler);
                return sequencer;
            }
        }

        /// <summary>
        /// 获取脚本上的C#组件
        /// </summary>
        /// <returns></returns>
        public ActorComponent GetMonoComponent()
        {
            return _GetMonoComponent(NativeHandler);
        }

        /// <summary>
        /// 查找Actor上的指定类型的组件
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
        public T GetComponent<T>() where T : ActorComponent , new()
        {
            IntPtr handler = _GetComponent(NativeHandler, typeof(T).Name);
            
            if (handler.ToInt64() == 0)
            {
                return null;
            }

            T ret = new T();
            ret.NativeHandler = handler;

            return ret;
        }

        /// <summary>
        /// 根据Tag查找Actor指定类型的组件
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="tag"></param>
        /// <returns></returns>
        public T GetComponentByTag<T>(string tag) where T : ActorComponent, new()
        {
            var handler = _GetComponentByTag(NativeHandler, typeof(T).Name, tag);
            if (handler.ToInt64() == 0)
            {
                return null;
            }
            T ret = new T();
            ret.NativeHandler = handler;
            return ret;
        }
        
        /// <summary>
        /// 销毁这个Actor
        /// </summary>
        public void Destroy()
        {
            _Destroy(NativeHandler);
            NativeHandler = IntPtr.Zero; 
        }

        public string Name
        {
            get { return _GetName(NativeHandler); }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static IntPtr _GetSequencer(IntPtr handler);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static bool _GetHiddenInGame(IntPtr handler);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static void _SetHiddenInGame(IntPtr handler, bool value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static IntPtr _GetSceneComponent(IntPtr handler);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static void _SetSceneComponent(IntPtr handler, IntPtr value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static ActorComponent _GetMonoComponent(IntPtr handler);
        [MethodImpl(MethodImplOptions.InternalCall)]      
        private extern static IntPtr _GetComponent(IntPtr handler, string type);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static IntPtr _GetComponentByTag(IntPtr handler, string type, string tag);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static void _Destroy(IntPtr handler);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static string _GetName(IntPtr handler);
    }
}
